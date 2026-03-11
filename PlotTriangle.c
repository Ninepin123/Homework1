#include<stdio.h>
#include<stdlib.h>
#include<math.h>

typedef long INT32;
typedef unsigned short int INT16;
typedef unsigned char U_CHAR;

#define UCH(x) ((int) (x))
#define GET_2B(array,offset)  ((INT16) UCH(array[offset]) + \
               (((INT16) UCH(array[offset+1])) << 8))
#define GET_4B(array,offset)  ((INT32) UCH(array[offset]) + \
               (((INT32) UCH(array[offset+1])) << 8) + \
               (((INT32) UCH(array[offset+2])) << 16) + \
               (((INT32) UCH(array[offset+3])) << 24))
#define FREAD(file,buf,sizeofbuf)  \
((size_t) fread((void *) (buf), (size_t) 1, (size_t) (sizeofbuf), (file)))

int ReadDataSize(char *name);
void ReadImageData(char *name, U_CHAR *bmpfileheader, U_CHAR *bmpinfoheader, U_CHAR *color_table, U_CHAR *data);

// 座標定義：原點(0,0)在圖片左上角，X 向下為正，Y 向右為正
#define START_X 300      // 頂點距離圖片上邊緣的距離 (向下)
#define START_Y 500      // 頂點距離圖片左邊緣的距離 (向右)
#define SIDE_LENGTH 400  // 三角形邊長

int main()
{
   FILE *output_file = 0 ;

   U_CHAR bmpfileheader1[14] = { 0 } ;
   U_CHAR bmpinfoheader1[40] = { 0 } ;
   U_CHAR *data1, *data2, color_table1[1024];

   INT32 biWidth = 0 ;
   INT32 biHeight = 0 ;

   int i, j, k, biWidth4;

   // ==== 計算三角形的幾何參數 ====
   double tri_height = SIDE_LENGTH * sqrt(3.0) / 2.0;  // 高度約 346.41
   int end_x = START_X + (int)tri_height;              // 底邊所在的邏輯垂直位置
   double half_base = SIDE_LENGTH / 2.0;               // 半底邊長度 (200)

   int dataSize = ReadDataSize("Fig2.20.bmp");
   data1 = (U_CHAR *)malloc(dataSize);
   if (data1 == NULL) {
      exit(0);
   }

   ReadImageData("Fig2.20.bmp", bmpfileheader1, bmpinfoheader1, color_table1, data1);
   biWidth    = GET_4B(bmpinfoheader1,4);
   biHeight   = GET_4B(bmpinfoheader1,8);

   data2 = (U_CHAR *)malloc(dataSize);
   if (data2 == NULL) {
      exit(0);
   }

   // 確保每行的 byte 數是 4 的倍數 (Padding)
   biWidth4 = ((biWidth * 1 + 3) / 4 * 4); 

   for (i = 0; i < biHeight; i++) // i 代表 BMP 陣列實體的列 (由下往上)
   {
      k = i * biWidth4;

      // 關鍵轉換：計算從「左上角」算起的垂直向下位移量 (邏輯 X 軸)
      int real_x = biHeight - 1 - i;

      for (j = 0; j < biWidth; j++) // j 代表水平向右的位移 (邏輯 Y 軸)
      {
         // 判斷 (real_x, j) 是否落在三角形的垂直高度範圍內
         if (real_x >= START_X && real_x <= end_x)
         {
            // 計算當前垂直深度下的左右邊界 (Y軸)
            double x_offset = real_x - START_X; 
            double y_left = START_Y - (x_offset * half_base / tri_height);
            double y_right = START_Y + (x_offset * half_base / tri_height);

            // 判斷像素是否落在左右邊界內
            if (j >= y_left && j <= y_right)
            {
               data2[k] = 255;  // 在三角形內部，設定為白色 (灰階 255)
            }
            else
            {
               data2[k] = data1[k];  // 在三角形左右外側，保持原圖像素
            }
         }
         else
         {
            data2[k] = data1[k];  // 不在三角形垂直範圍內，保持原圖像素
         }

         k = k + 1;
      }
   }

   /* 寫入新檔案 */
   if( ( output_file = fopen("triangle_final.bmp","wb") ) == NULL ){
      fprintf(stderr,"Output file can't open.\n");
      exit(0);
   }

   fwrite(bmpfileheader1, sizeof(bmpfileheader1), 1, output_file);
   fwrite(bmpinfoheader1, sizeof(bmpinfoheader1), 1, output_file);
   fwrite(color_table1, 1024, 1, output_file);
   fwrite(data2, biWidth4 * biHeight * 1, 1, output_file);

   fclose(output_file);

   free(data1);
   free(data2);

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

   biWidth          =   GET_4B(bmpinfoheader,4);
   biHeight         =   GET_4B(bmpinfoheader,8);
   BitCount         =   GET_2B(bmpinfoheader,14);

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

   FileSize          =   GET_4B(bmpfileheader,2);
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
