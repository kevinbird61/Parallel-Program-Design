#include <mpi.h>
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"

using namespace std;

//�w�q���ƹB�⪺����
#define NSmooth 1000

/*********************************************************/
/*�ܼƫŧi�G                                             */
/*  bmpHeader    �G BMP�ɪ����Y                          */
/*  bmpInfo      �G BMP�ɪ���T                          */
/*  **BMPSaveData�G �x�s�n�Q�g�J���������               */
/*  **BMPData    �G �Ȯ��x�s�n�Q�g�J���������           */
/*********************************************************/
BMPHEADER bmpHeader;                        
BMPINFO bmpInfo;
RGBTRIPLE *BMPSaveData = NULL;                                               
RGBTRIPLE *BMPData = NULL;                                                   
/*********************************************************/
/*��ƫŧi�G                                             */
/*  readBMP    �G Ū�����ɡA�ç⹳������x�s�bBMPSaveData*/
/*  saveBMP    �G �g�J���ɡA�ç⹳�����BMPSaveData�g�J  */
/*  swap       �G �洫�G�ӫ���                           */
/*  **alloc_memory�G �ʺA���t�@��Y * X�x�}               */
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
/*�ܼƫŧi�G                                             */
/*  *infileName  �G Ū���ɦW                             */
/*  *outfileName �G �g�J�ɦW                             */
/*  startwtime   �G �O���}�l�ɶ�                         */
/*  endwtime     �G �O�������ɶ�                         */
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
    //�ۭq�qtype
	MPI_Datatype type = MPI_UNSIGNED_CHAR;
	int blocklength = 3;
	MPI_Aint displace=0;
	MPI_Type_struct(1,&blocklength,&displace,&type,&Pixel);
	MPI_Type_commit(&Pixel);
	//Allocate the memory
    BlockSize = (int*)malloc(comm_sz*sizeof(int));
	BlockHeight = (int*)malloc(comm_sz*sizeof(int));
	displs = (int*)malloc(comm_sz*sizeof(int));
	//Ū���ɮ�
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
	//�O���}�l�ɶ�
    MPI_Barrier(MPI_COMM_WORLD);
	startwtime = MPI_Wtime();
	//�ʺA���t�O���鵹�Ȧs�Ŷ�
    BMPData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
//===========================================================�B�z====================================================
        //�i��h�������ƹB��
	for(int count = 0; count < NSmooth ; count ++){
		// �i����t - MPI_Scatterv
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
		//�i�業�ƹB��
		for(int i = 1; i< BlockHeight[my_rank]+1; i++){
			for(int j =0; j<bmpInfo.biWidth ; j++){
				/*********************************************************/
				/*�]�w�W�U���k��������m                                 */
				/*********************************************************/
				int Top = i-1;
				int Down = i+1;
				int Left = j>0 ? j-1 : bmpInfo.biWidth-1;
				int Right = j<bmpInfo.biWidth-1 ? j+1 : 0;
				/*********************************************************/
				/*�P�W�U���k�����������A�å|�ˤ��J                       */
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
		// �i��X�� - MPI_Gatherv
		MPI_Gatherv(BMPData,BlockSize[my_rank],Pixel,BMPSaveData,BlockSize,displs,Pixel,0,MPI_COMM_WORLD);
    }
	
//===========================================================�B�z==================================================== 
 	//�g�J�ɮ�
    if(my_rank==0){
    if ( saveBMP( outfileName ) )
          cout << "Save file successfully!!" << endl;
    else
          cout << "Save file fails!!" << endl;
	//�o�쵲���ɶ��A�æL�X����ɶ�
    //MPI_Barrier(MPI_COMM_WORLD);    
    endwtime = MPI_Wtime();
   	cout << "The execution time = "<< endwtime-startwtime <<endl ;
	}
 	// MPI_Final 
 	MPI_Finalize();
    return 0;
}

/*********************************************************/
/* Ū������                                              */
/*********************************************************/
int readBMP(char *fileName)
{
	//�إ߿�J�ɮת���	
        ifstream bmpFile( fileName, ios::in | ios::binary );
 
        //�ɮ׵L�k�}��
        if ( !bmpFile ){
                cout << "It can't open file!!" << endl;
                return 0;
        }
 
        //Ū��BMP���ɪ����Y���
    	bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );
 
        //�P�M�O�_��BMP����
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }
 
        //Ū��BMP����T
        bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );
        
        //�P�_�줸�`�׬O�_��24 bits
        if ( bmpInfo.biBitCount != 24 ){
                cout << "The file is not 24 bits!!" << endl;
                return 0;
        }

        //�ץ��Ϥ����e�׬�4������
        while( bmpInfo.biWidth % 4 != 0 )
        	bmpInfo.biWidth++;

        //�ʺA���t�O����
        BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
        
        //Ū���������
    	//for(int i = 0; i < bmpInfo.biHeight; i++)
        //	bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
		bmpFile.read( (char* )BMPSaveData, bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);
	
        //�����ɮ�
        bmpFile.close();
 
        return 1;
 
}
/*********************************************************/
/* �x�s����                                              */
/*********************************************************/
int saveBMP( char *fileName)
{
 	//�P�M�O�_��BMP����
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }
        
 	//�إ߿�X�ɮת���
        ofstream newFile( fileName,  ios:: out | ios::binary );
 
        //�ɮ׵L�k�إ�
        if ( !newFile ){
                cout << "The File can't create!!" << endl;
                return 0;
        }
 	
        //�g�JBMP���ɪ����Y���
        newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

	//�g�JBMP����T
        newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

        //�g�J�������
        //for( int i = 0; i < bmpInfo.biHeight; i++ )
        //        newFile.write( ( char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE) );
        newFile.write( ( char* )BMPSaveData, bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

        //�g�J�ɮ�
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
/* ���t�O����G�^�Ǭ�Y*X���x�}                           */
/*********************************************************/
RGBTRIPLE *alloc_memory(int Y, int X )
{        
	//�إߪ��׬�Y�����а}�C
	RGBTRIPLE *temp = new RGBTRIPLE [ Y * X ];
    memset( temp, 0, sizeof( RGBTRIPLE ) * Y * X);
	
    return temp;
 
}
/*********************************************************/
/* �洫�G�ӫ���                                          */
/*********************************************************/
void swap(RGBTRIPLE **a, RGBTRIPLE **b)
{
	RGBTRIPLE *temp;
	temp = *a;
	*a = *b;
	*b = temp;
}

