#include<stdio.h>
#include<stdlib.h>
#include<math.h>

typedef long INT32;
typedef unsigned short int INT16;
typedef unsigned char U_CHAR;

#define UCH(x)	((int) (x))
#define GET_2B(array,offset)  ((INT16) UCH(array[offset]) + \
			       (((INT16) UCH(array[offset+1])) << 8))
#define GET_4B(array,offset)  ((INT32) UCH(array[offset]) + \
			       (((INT32) UCH(array[offset+1])) << 8) + \
			       (((INT32) UCH(array[offset+2])) << 16) + \
			       (((INT32) UCH(array[offset+3])) << 24))
#define PUT_2B(array,offset,value)  \
   (array[offset] = (U_CHAR) ((value) & 0xFF), \
    array[offset+1] = (U_CHAR) (((value) >> 8) & 0xFF))
#define PUT_4B(array,offset,value)  \
   (array[offset] = (U_CHAR) ((value) & 0xFF), \
    array[offset+1] = (U_CHAR) (((value) >> 8) & 0xFF), \
    array[offset+2] = (U_CHAR) (((value) >> 16) & 0xFF), \
    array[offset+3] = (U_CHAR) (((value) >> 24) & 0xFF))
#define FREAD(file,buf,sizeofbuf)  \
  ((size_t) fread((void *) (buf), (size_t) 1, (size_t) (sizeofbuf), (file)))

int ReadDataSize(char *name);
void ReadImageData(char *name, U_CHAR *bmpfileheader, U_CHAR *bmpinfoheader, U_CHAR *color_table, U_CHAR *data);

int main()
{
   FILE *output_file = 0 ;

   U_CHAR bmpfileheader1[14] = { 0 } ;
   U_CHAR bmpinfoheader1[40] = { 0 } ;
   U_CHAR *data1, *data2, color_table1[1024];

   INT32 biWidth = 0 ;
   INT32 biHeight = 0 ;

   int i, j, k;
   int biWidth4_old, biWidth4_new;
   INT32 new_width, new_height, new_file_size, new_image_size;

   i = ReadDataSize("Fig2.20.bmp");
   data1 = (U_CHAR *)malloc( i );
   if (data1 == NULL) {
      exit(0);
   }

   ReadImageData("Fig2.20.bmp", bmpfileheader1, bmpinfoheader1, color_table1, data1);
   biWidth           =   GET_4B(bmpinfoheader1,4);
   biHeight          =   GET_4B(bmpinfoheader1,8);

   // After 90-degree clockwise rotation: width and height are swapped
   new_width = biHeight;
   new_height = biWidth;

   // Calculate row padding for old and new images
   biWidth4_old = ((biWidth * 1 + 3) / 4 * 4);
   biWidth4_new = ((new_width * 1 + 3) / 4 * 4);

   // Allocate memory for rotated image
   data2 = (U_CHAR *)malloc( biWidth4_new * new_height );
   if (data2 == NULL) {
      free(data1);
      exit(0);
   }

   // Initialize output data to 0 (black)
   for (i = 0; i < biWidth4_new * new_height; i++) {
      data2[i] = 0;
   }

   // Perform 90-degree clockwise rotation about the center
   // For pixel at (x, y) in original image:
   //   90° clockwise: new_x = y, new_y = width - 1 - x
   for (i = 0; i < biHeight; i++)
   {
       k = i * biWidth4_old;
       for (j = 0; j < biWidth; j++)
       {
           int new_x = i;
           int new_y = biWidth - 1 - j;

           // Copy pixel value to rotated position
           data2[new_y * biWidth4_new + new_x] = data1[k + j];
       }
   }

   // Update BMP headers for the rotated image
   new_image_size = biWidth4_new * new_height;
   new_file_size = 14 + 40 + 1024 + new_image_size;

   // Update file header
   PUT_4B(bmpfileheader1, 2, new_file_size);

   // Update info header
   PUT_4B(bmpinfoheader1, 4, new_width);   // biWidth
   PUT_4B(bmpinfoheader1, 8, new_height);  // biHeight
   PUT_4B(bmpinfoheader1, 20, new_image_size);  // biSizeImage

   /* Write new file */
   if( ( output_file = fopen("Fig2.20_rotated90.bmp","wb") ) == NULL ){
      fprintf(stderr,"Output file can't open.\n");
      free(data1);
      free(data2);
      exit(0);
   }

   fwrite(bmpfileheader1, sizeof(bmpfileheader1), 1, output_file);
   fwrite(bmpinfoheader1, sizeof(bmpinfoheader1), 1, output_file);
   fwrite(color_table1, 1024, 1, output_file);
   fwrite(data2, biWidth4_new * new_height, 1, output_file);

   fclose (output_file);

   free(data1);
   free(data2);

   printf("Image rotated 90 degrees clockwise successfully!\n");
   printf("Original: %ld x %ld\n", biWidth, biHeight);
   printf("Rotated:  %ld x %ld\n", new_width, new_height);
   printf("Output file: Fig2.20_rotated90.bmp\n");

   return 0;
}

int ReadDataSize(char *name)
{
   FILE *input_file = 0 ;
   U_CHAR bmpfileheader[14] = { 0 } ;
   U_CHAR bmpinfoheader[40] = { 0 } ;

   INT32 biWidth = 0 ;
   INT32 biHeight = 0 ;
   INT16 BitCount = 0 ;

   if( ( input_file = fopen(name,"rb") ) == NULL ){
      fprintf(stderr,"File can't open.\n");
      exit(0);
   }

   FREAD(input_file,bmpfileheader,14);
   FREAD(input_file,bmpinfoheader,40);

   if (GET_2B(bmpfileheader,0) == 0x4D42) /* 'BM' */
      fprintf(stdout,"BMP file.\n");
   else{
      fprintf(stdout,"Not bmp file.\n");
      exit(0);
   }

   biWidth           =   GET_4B(bmpinfoheader,4);
   biHeight          =   GET_4B(bmpinfoheader,8);
   BitCount          =   GET_2B(bmpinfoheader,14);

   if (BitCount != 8) {
      fprintf(stderr,"Not a 8-bit file.\n");
      fclose (input_file);
      exit(0);
   }

   fclose (input_file);

   return ((biWidth*1 +3)/4 *4)*biHeight*1;
}

void ReadImageData(char *name, U_CHAR *bmpfileheader, U_CHAR *bmpinfoheader, U_CHAR *color_table, U_CHAR *data)
{
   FILE *input_file = 0 ;

   INT32 FileSize = 0 ;
   INT32 bfOffBits =0 ;
   INT32 headerSize =0 ;
   INT32 biWidth = 0 ;
   INT32 biHeight = 0 ;
   INT16 biPlanes = 0 ;
   INT16 BitCount = 0 ;
   INT32 biCompression = 0 ;
   INT32 biImageSize = 0;
   INT32 biXPelsPerMeter = 0 ,biYPelsPerMeter = 0 ;
   INT32 biClrUsed = 0 ;
   INT32 biClrImp = 0 ;

   if( ( input_file = fopen(name,"rb") ) == NULL ){
      fprintf(stderr,"File can't open.\n");
      exit(0);
   }

   FREAD(input_file,bmpfileheader,14);
   FREAD(input_file,bmpinfoheader,40);

   if (GET_2B(bmpfileheader,0) == 0x4D42) /* 'BM' */
      fprintf(stdout,"BMP file.\n");
   else{
      fprintf(stdout,"Not bmp file.\n");
      exit(0);
   }

   FileSize           =   GET_4B(bmpfileheader,2);
   bfOffBits         =   GET_4B(bmpfileheader,10);
   headerSize      =   GET_4B(bmpinfoheader,0);
   biWidth           =   GET_4B(bmpinfoheader,4);
   biHeight          =   GET_4B(bmpinfoheader,8);
   biPlanes          =   GET_2B(bmpinfoheader,12);
   BitCount          =   GET_2B(bmpinfoheader,14);
   biCompression   =   GET_4B(bmpinfoheader,16);
   biImageSize      =   GET_4B(bmpinfoheader,20);
   biXPelsPerMeter =   GET_4B(bmpinfoheader,24);
   biYPelsPerMeter =   GET_4B(bmpinfoheader,28);
   biClrUsed         =   GET_4B(bmpinfoheader,32);
   biClrImp          =   GET_4B(bmpinfoheader,36);

   printf("FileSize = %ld \n"
	"DataOffset = %ld \n"
           "HeaderSize = %ld \n"
	"Width = %ld \n"
	"Height = %ld \n"
	"Planes = %d \n"
	"BitCount = %d \n"
	"Compression = %ld \n"
	"ImageSize = %ld \n"
	"XpixelsPerM = %ld \n"
	"YpixelsPerM = %ld \n"
	"ColorsUsed = %ld \n"
	"ColorsImportant = %ld \n",FileSize,bfOffBits,headerSize,biWidth,biHeight,biPlanes,
	BitCount,biCompression,biImageSize,biXPelsPerMeter,biYPelsPerMeter,biClrUsed,biClrImp);

   if (BitCount != 8) {
      fprintf(stderr,"Not a 8-bit file.\n");
      fclose (input_file);
      exit(0);
   }

   FREAD(input_file,color_table,1024);

   fseek(input_file,bfOffBits,SEEK_SET);
   FREAD(input_file,data,((biWidth*1 +3)/4 *4)*biHeight*1);

   fclose (input_file);
}
