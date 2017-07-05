#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#ifdef WIN32
	#include<io.h>
	#include<direct.h>
	#define md _mkdir
	#define getcwd _getcwd
#else
	#include<unistd.h>
	#include<sys/stat.h>
	#define md(fName) mkdir(fName,0777)
#endif
size_t iGetFileSize(FILE *fn){
	size_t ret=-1;
	if(fn!=NULL){
		fseek(fn,0,SEEK_END);
		ret = ftell(fn);
	}
	return ret;
}
int iFileErr(FILE* fN){
	extern int errno;
	if(!fN && ferror(fN)){
		switch(errno){
			case 13: printf("Permission Deny!");break;
			case 2: printf("File Not Found!");break;
			default: printf("Unknow error code %d",errno);
		}
		return errno;
	}
	return 0;
}
unsigned long long iReadLEInt(FILE* fN, int iCount){
	unsigned char cBit;
	int i = 0;
	unsigned long long iInt = 0;
	for(;i<iCount;++i){
		cBit = fgetc(fN);
		iInt |= cBit << i*8;
	}
	return iInt;
}
char* sReadName(FILE* fN, int iCount){
	int i = 0;
	char* sName = (char*)malloc(iCount/2+1);
	//memset(sName,iCount*2+1,0);
	for(;i<iCount;i+=2){
		sName[i/2] = fgetc(fN);
		fgetc(fN);
	}
	sName[iCount/2] = '\0';
	return sName;
}
int iWrite(FILE* fIn, const char* fName, unsigned long iOffset, unsigned long iFSize){
	extern int errno;
	FILE* fOut;
	fOut = fopen(fName,"wb");
	if(iFileErr(fOut))return errno;
	fseek(fIn,iOffset,SEEK_SET);
	//violence
	while(iFSize--/* && !ferror(fIn) && !ferror(fOut)*/)fputc(fgetc(fIn),fOut);
	/*//Load sub-file to memory
	char* buffer = (char*)malloc(iFSize*sizeof(char));
	fread(buffer,sizeof(char),iFSize,fIn);
	fwrite(buffer,sizeof(char),iFSize,fOut);
	free(buffer);
	*/
	fclose(fOut);
	return errno;
}
//#define debug
#ifdef debug
const int argc = 2;
const char *argv[] = {"mail.c.exe","__bgm.pck"};
int main() {
#else
int main(int argc, const char* argv[]) {
#endif
	extern int errno;
	FILE *fPck,*fOut;
	size_t iLen,iFSize;
	char sOutDir[1024],*sSubFN,*buf;
	unsigned int iL1,iCount,iW,iH,iSubLen,i;
	unsigned long lSubSize,lOffset,lCurSL,lCurSN,lCurSO;
	
	//Open and check the pck file
	if(argc<2){
		printf("No Input File");
		return 1;
	}
	if(access(argv[1], F_OK)){
		printf("No Input File");
		return 1;
	}
	iLen = strlen(argv[1]) - 4;//(strlen(".pck") - 1);
	if(strcmp(argv[1] + iLen,".pck")){
		printf("NOT Require File!");
		return 2;
	}
	fPck = fopen(argv[1],"rb");
	if(iFileErr(fPck))return errno;
	iFSize = iGetFileSize(fPck);
	rewind(fPck);

	//Make the output dir
	strncpy(sOutDir,argv[1],iLen);
	//sOutDir[iLen] = '\0';
	sOutDir[iLen] = '\\';
	sOutDir[++iLen] = '\0';
	if(!access(sOutDir, F_OK)){
		if(access(sOutDir, W_OK)){
			printf("Dir %s cannot be read",sOutDir);
			return 3;
		}
	}else{
		md(sOutDir);
	}

	//Read and divide the pck
	iL1 = iReadLEInt(fPck,4);
	if(iL1 != 1){
		printf("Unknow type");
		return 4;
	}
	iCount = iReadLEInt(fPck,4);
	printf("%d files\n",iCount);
	iW = iReadLEInt(fPck,4);
	iH = iReadLEInt(fPck,4);
	printf("width:%d heigh:%d (unknow data)\n",iW,iH);
	iReadLEInt(fPck,4*4);
	lCurSL = ftell(fPck);
	lCurSN = lCurSL + iCount*4;
	for(i=iSubLen=0;i<iCount;++i)iSubLen += iReadLEInt(fPck,4);
	//lCurSN = ftell(fPck);
	lCurSO = lCurSN + iSubLen;
	for(i=0;i<iCount;++i){
		fseek(fPck,lCurSL,SEEK_SET);
		iSubLen = iReadLEInt(fPck,4);
		lCurSL += 4;
		//lCurSL = ftell(fPck);
		fseek(fPck,lCurSN,SEEK_SET);
		sSubFN = sReadName(fPck,iSubLen);
			printf("NO.%03d:%s\t",i,sSubFN);
		strcpy(sOutDir + iLen,sSubFN);
			printf(".");
		free(sSubFN);
			printf(".");
		lCurSN += iSubLen;
			printf(".");
		//lCurSN = ftell(fPck);
		fseek(fPck,lCurSO,SEEK_SET);
			printf(".");
		lOffset = iReadLEInt(fPck,8);
			printf(".");
		lSubSize = iReadLEInt(fPck,8);
			printf(".");
		lCurSO += 16;
			printf(".");
		//lCurSO = ftell(fPck);
		iWrite(fPck,sOutDir,lOffset,lSubSize);
			printf("done\n");
	}
	fclose(fPck);
	printf("Finished\n");
	sOutDir[iLen] = '\0';
	printf("Pck was been unpacked in %s\n",sOutDir);
	return 0;
}