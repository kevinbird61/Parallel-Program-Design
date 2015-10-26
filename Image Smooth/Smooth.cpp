#include <mpi.h>
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"

using namespace std;

//定義平滑運算的次數
#define NSmooth 1000

/*********************************************************/
/*變數宣告：                                             */
/*  bmpHeader    ： BMP檔的標頭                          */
/*  bmpInfo      ： BMP檔的資訊                          */
/*  **BMPSaveData： 儲存要被寫入的像素資料               */
/*  **BMPData    ： 暫時儲存要被寫入的像素資料           */
/*********************************************************/
BMPHEADER bmpHeader;                        
BMPINFO bmpInfo;
RGBTRIPLE *BMPSaveData = NULL;                                               
RGBTRIPLE *BMPData = NULL;                                                   
/*********************************************************/
/*函數宣告：                                             */
/*  readBMP    ： 讀取圖檔，並把像素資料儲存在BMPSaveData*/
/*  saveBMP    ： 寫入圖檔，並把像素資料BMPSaveData寫入  */
/*  swap       ： 交換二個指標                           */
/*  **alloc_memory： 動態分配一個Y * X矩陣               */
/*********************************************************/
int readBMP( char *fileName);        //read file
int saveBMP( char *fileName);        //save file
RGBTRIPLE fetchloc(RGBTRIPLE *arr, int Y, int X);  // get array element by address
void swap(RGBTRIPLE **a, RGBTRIPLE **b);
RGBTRIPLE *alloc_memory( int Y, int X );        //allocate memory - change to allocate one dimension array
MPI_Datatype Pixel;			// For User-defined data type

int main(int argc,char *argv[])
{
/*********************************************************/
/*變數宣告：                                             */
/*  *infileName  ： 讀取檔名                             */
/*  *outfileName ： 寫入檔名                             */
/*  startwtime   ： 記錄開始時間                         */
/*  endwtime     ： 記錄結束時間                         */
/*********************************************************/
	char *infileName = "input.bmp";
    char *outfileName = "parallel.bmp";
	double startwtime = 0.0, endwtime=0.0;
	// For MPI_COMM_WORLD
	int my_rank,comm_sz;
	// displs - Block displacement ; BlockHeight - Block's Height ; BlockSize - Block's Size
	int *BlockSize,*displs,*BlockHeight;
	//Initialize MPI
    MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&comm_sz);
    //自訂義type
	MPI_Datatype type = MPI_UNSIGNED_CHAR;
	int blocklength = 3;
	MPI_Aint displace=0;
	MPI_Type_struct(1,&blocklength,&displace,&type,&Pixel);
	MPI_Type_commit(&Pixel);
	//Allocate the memory
    BlockSize = (int*)malloc(comm_sz*sizeof(int));
	BlockHeight = (int*)malloc(comm_sz*sizeof(int));
	displs = (int*)malloc(comm_sz*sizeof(int));
	//讀取檔案
	if(my_rank==0){
    if ( readBMP( infileName) )
           cout << "Read file successfully!!" << endl;
    else 
           cout << "Read file fails!!" << endl;
    }
	// Broadcast the bmp Height and bmp Width
    MPI_Bcast(&bmpInfo.biHeight,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&bmpInfo.biWidth,1,MPI_INT,0,MPI_COMM_WORLD);
    if(my_rank!=0){
		BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
	}
    // Calculate each block's displacement,Height and Size
    for(int i=0;i<comm_sz;i++){
		if(bmpInfo.biHeight%comm_sz!=0&&i==comm_sz-1){
			BlockSize[i] = ((bmpInfo.biHeight/comm_sz)+bmpInfo.biHeight%comm_sz)*bmpInfo.biWidth;
			BlockHeight[i] = (bmpInfo.biHeight/comm_sz)+bmpInfo.biHeight%comm_sz;
		}
		else{
			BlockSize[i] = ((bmpInfo.biHeight/comm_sz))*bmpInfo.biWidth;
			BlockHeight[i] = (bmpInfo.biHeight/comm_sz);
		}
		displs[i] = ((bmpInfo.biHeight/comm_sz)*bmpInfo.biWidth)*i;
	}
	//記錄開始時間
    MPI_Barrier(MPI_COMM_WORLD);
	startwtime = MPI_Wtime();
	//動態分配記憶體給暫存空間
    BMPData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
//===========================================================處理====================================================
        //進行多次的平滑運算
	for(int count = 0; count < NSmooth ; count ++){
		// 進行分配 - MPI_Scatterv
		MPI_Scatterv(BMPSaveData,BlockSize,displs,Pixel,BMPData,BlockSize[my_rank],Pixel,0,MPI_COMM_WORLD);
		// Shifting the BMPData for the Boundary buffer(Up/Down)
        for(int k = BlockSize[my_rank]-1;k>=0;k--){
            BMPData[k+bmpInfo.biWidth] = BMPData[k]; // Shift to Next line (One dimension array)            
        }
        // Send and Recv the Upper Bound
        MPI_Send(&BMPData[BlockSize[my_rank]],bmpInfo.biWidth,Pixel,(my_rank==comm_sz-1?0:my_rank+1),0,MPI_COMM_WORLD);
        MPI_Recv(&BMPData[0],bmpInfo.biWidth,Pixel,(my_rank>0?my_rank-1:comm_sz-1),0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        // Send and Recv the Lower Bound
        MPI_Send(&BMPData[bmpInfo.biWidth],bmpInfo.biWidth,Pixel,(my_rank>0?my_rank-1:comm_sz-1),1,MPI_COMM_WORLD);
        MPI_Recv(&BMPData[BlockSize[my_rank]+bmpInfo.biWidth],bmpInfo.biWidth,Pixel,(my_rank>0?(my_rank==comm_sz-1?0:my_rank+1):1),1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		//進行平滑運算
		for(int i = 1; i< BlockHeight[my_rank]+1; i++){
			for(int j =0; j<bmpInfo.biWidth ; j++){
				/*********************************************************/
				/*設定上下左右像素的位置                                 */
				/*********************************************************/
				int Top = i-1;
				int Down = i+1;
				int Left = j>0 ? j-1 : bmpInfo.biWidth-1;
				int Right = j<bmpInfo.biWidth-1 ? j+1 : 0;
				/*********************************************************/
				/*與上下左右像素做平均，並四捨五入                       */
				/*********************************************************/
				BMPSaveData[i*bmpInfo.biWidth+j].rgbBlue =  (double) (fetchloc(BMPData,i,j).rgbBlue+fetchloc(BMPData,Top,j).rgbBlue+fetchloc(BMPData,Down,j).rgbBlue+fetchloc(BMPData,i,Left).rgbBlue+fetchloc(BMPData,i,Right).rgbBlue)/5+0.5;
				BMPSaveData[i*bmpInfo.biWidth+j].rgbGreen =  (double) (fetchloc(BMPData,i,j).rgbGreen+fetchloc(BMPData,Top,j).rgbGreen+fetchloc(BMPData,Down,j).rgbGreen+fetchloc(BMPData,i,Left).rgbGreen+fetchloc(BMPData,i,Right).rgbGreen)/5+0.5;
				BMPSaveData[i*bmpInfo.biWidth+j].rgbRed =  (double) (fetchloc(BMPData,i,j).rgbRed+fetchloc(BMPData,Top,j).rgbRed+fetchloc(BMPData,Down,j).rgbRed+fetchloc(BMPData,i,Left).rgbRed+fetchloc(BMPData,i,Right).rgbRed)/5+0.5;
			}
		}
		//Swap
		swap(&BMPData,&BMPSaveData);
        // Shift Back to original location
        for(int l=0;l<BlockSize[my_rank];l++){
            BMPData[l] = BMPData[l+bmpInfo.biWidth];
        }
		// 進行合併 - MPI_Gatherv
		MPI_Gatherv(BMPData,BlockSize[my_rank],Pixel,BMPSaveData,BlockSize,displs,Pixel,0,MPI_COMM_WORLD);
    }
	
//===========================================================處理==================================================== 
 	//寫入檔案
    if(my_rank==0){
    if ( saveBMP( outfileName ) )
          cout << "Save file successfully!!" << endl;
    else
          cout << "Save file fails!!" << endl;
	//得到結束時間，並印出執行時間
    //MPI_Barrier(MPI_COMM_WORLD);    
    endwtime = MPI_Wtime();
   	cout << "The execution time = "<< endwtime-startwtime <<endl ;
	}
 	// MPI_Final 
 	MPI_Finalize();
    return 0;
}

/*********************************************************/
/* 讀取圖檔                                              */
/*********************************************************/
int readBMP(char *fileName)
{
	//建立輸入檔案物件	
        ifstream bmpFile( fileName, ios::in | ios::binary );
 
        //檔案無法開啟
        if ( !bmpFile ){
                cout << "It can't open file!!" << endl;
                return 0;
        }
 
        //讀取BMP圖檔的標頭資料
    	bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );
 
        //判決是否為BMP圖檔
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }
 
        //讀取BMP的資訊
        bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );
        
        //判斷位元深度是否為24 bits
        if ( bmpInfo.biBitCount != 24 ){
                cout << "The file is not 24 bits!!" << endl;
                return 0;
        }

        //修正圖片的寬度為4的倍數
        while( bmpInfo.biWidth % 4 != 0 )
        	bmpInfo.biWidth++;

        //動態分配記憶體
        BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
        
        //讀取像素資料
    	//for(int i = 0; i < bmpInfo.biHeight; i++)
        //	bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
		bmpFile.read( (char* )BMPSaveData, bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);
	
        //關閉檔案
        bmpFile.close();
 
        return 1;
 
}
/*********************************************************/
/* 儲存圖檔                                              */
/*********************************************************/
int saveBMP( char *fileName)
{
 	//判決是否為BMP圖檔
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }
        
 	//建立輸出檔案物件
        ofstream newFile( fileName,  ios:: out | ios::binary );
 
        //檔案無法建立
        if ( !newFile ){
                cout << "The File can't create!!" << endl;
                return 0;
        }
 	
        //寫入BMP圖檔的標頭資料
        newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

	//寫入BMP的資訊
        newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

        //寫入像素資料
        //for( int i = 0; i < bmpInfo.biHeight; i++ )
        //        newFile.write( ( char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE) );
        newFile.write( ( char* )BMPSaveData, bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

        //寫入檔案
        newFile.close();
 
        return 1;
 
}
/*********************************************************/
/*  Retrive location                                      */
/*********************************************************/
RGBTRIPLE fetchloc(RGBTRIPLE *arr, int Y, int X){
	return arr[bmpInfo.biWidth * Y + X ];
}
/*********************************************************/
/* 分配記憶體：回傳為Y*X的矩陣                           */
/*********************************************************/
RGBTRIPLE *alloc_memory(int Y, int X )
{        
	//建立長度為Y的指標陣列
	RGBTRIPLE *temp = new RGBTRIPLE [ Y * X ];
    memset( temp, 0, sizeof( RGBTRIPLE ) * Y * X);
	
    return temp;
 
}
/*********************************************************/
/* 交換二個指標                                          */
/*********************************************************/
void swap(RGBTRIPLE **a, RGBTRIPLE **b)
{
	RGBTRIPLE *temp;
	temp = *a;
	*a = *b;
	*b = temp;
}

