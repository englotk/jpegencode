#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <cstdlib>
#include <termios.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <ctype.h>
#include <inttypes.h>

#include <linux/serial.h>
#include <sys/ioctl.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

//#define THREAD_DISPLAY

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)

#ifdef THREAD_DISPLAY
pthread_t tid[2];
#endif

FILE* pFile; // the output file
FILE* dFile; // use to keep the received pkt in ASCII hex format for debug purpose mainly

char rcvFile[80];
char rcvFile_tmp[80];

char* originalFile;
char* displayFile;

// display if set to true
bool pktDisplay=false;
// write a pkt list file if set to true
bool pktFile=false;
// write a PGM file if set to true 
bool pgmFile=false;
// write a PGM binary file if set to true 
bool pgmBFile=false;
// use framing bytes
bool framing_mode=false;
// display original or received .dat file?
bool display_received_dat=false;
// vertically flip or not
bool vflip=false;
// wait for key press
bool key_press_option=true;

uint8_t* myData=NULL;
uint8_t myData_len;
int qualityFactor=50;
int fileSN=1;
int nbPktRcv=0;

timer_t timerid;
struct sigevent sev;
struct itimerspec its;
long long freq_nanosecs;
uint8_t display_timer_sec=5;
sigset_t mask;
struct sigaction sa;

timespec firstPkt, lastPkt, currentPkt;

timespec diff(timespec start, timespec end)
{
        timespec temp;
        if ((end.tv_nsec-start.tv_nsec)<0) {
                temp.tv_sec = end.tv_sec-start.tv_sec-1;
                temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
        } else {
                temp.tv_sec = end.tv_sec-start.tv_sec;
                temp.tv_nsec = end.tv_nsec-start.tv_nsec;
        }
        return temp;
}

uint8_t str2hex(char* str)
{
        int aux=0, aux2=0;

        if( (*str>='0') && (*str<='9') )
        {
                aux=*str++-'0';
        }
        else if( (*str>='A') && (*str<='F') )
        {
                aux=*str++-'A'+10;
        }
        if( (*str>='0') && (*str<='9') )
        {
                aux2=*str-'0';
        }
        else if( (*str>='A') && (*str<='F') )
        {
                aux2=*str-'A'+10;
        }
        return aux*16+aux2;
}

// begin from CRAN

/************************************************************************
 *									*
 *	Fast DCT for image compression scheme				*
 *									*
 *	Author: Vincent LECUIRE, CRAN UMR 7039, Nancy-Université, CNRS	*
 *	Date: march, 16 2012						*
 *									*
 ************************************************************************/

#include "bmp.h"
#include "mqc.h"

#define CRAN_ENCODING_IMAGE_TYPE -1

#define a4   1.38703984532215
#define a7  -0.275899379282943
#define a47  0.831469612302545
#define a5   1.17587560241936
#define a6  -0.785694958387102
#define a56  0.98078528040323
#define a2   1.84775906502257
#define a3   0.765366864730179
#define a23  0.541196100146197
#define a32  1.306562964876376
#define rc2  1.414213562373095

double OriginalLuminanceJPEGTable[8][8] = {
  16.0,  11.0,  10.0,  16.0,  24.0,  40.0,  51.0,  61.0,
  12.0,  12.0,  14.0,  19.0,  26.0,  58.0,  60.0,  55.0,
  14.0,  13.0,  16.0,  24.0,  40.0,  57.0,  69.0,  56.0,
  14.0,  17.0,  22.0,  29.0,  51.0,  87.0,  80.0,  62.0,
  18.0,  22.0,  37.0,  56.0,  68.0, 109.0, 103.0,  77.0,
  24.0,  35.0,  55.0,  64.0,  81.0, 104.0, 113.0,  92.0,
  49.0,  64.0,  78.0,  87.0, 103.0, 121.0, 120.0, 101.0,
  72.0,  92.0,  95.0,  98.0, 112.0, 100.0, 103.0,  99.0
};

double LuminanceJPEGTable[8][8] = {
  16.0,  11.0,  10.0,  16.0,  24.0,  40.0,  51.0,  61.0,
  12.0,  12.0,  14.0,  19.0,  26.0,  58.0,  60.0,  55.0,
  14.0,  13.0,  16.0,  24.0,  40.0,  57.0,  69.0,  56.0,
  14.0,  17.0,  22.0,  29.0,  51.0,  87.0,  80.0,  62.0,
  18.0,  22.0,  37.0,  56.0,  68.0, 109.0, 103.0,  77.0,
  24.0,  35.0,  55.0,  64.0,  81.0, 104.0, 113.0,  92.0,
  49.0,  64.0,  78.0,  87.0, 103.0, 121.0, 120.0, 101.0,
  72.0,  92.0,  95.0,  98.0, 112.0, 100.0, 103.0,  99.0
};

struct position { int row;	int col;
	} ZigzagCoordinates[8*8]=	// Matrice Zig-Zag
	{0, 0, 0, 1, 1, 0, 2, 0, 1, 1, 0, 2, 0, 3, 1, 2, 2, 1, 3, 0,
	 4, 0, 3, 1, 2, 2, 1, 3, 0, 4, 0, 5, 1, 4, 2, 3, 3, 2, 4, 1,
	 5, 0, 6, 0, 5, 1, 4, 2, 3, 3, 2, 4, 1, 5, 0, 6, 0, 7, 1, 6,
	 2, 5, 3, 4, 4, 3, 5, 2, 6, 1, 7, 0, 7, 1, 6, 2, 5, 3, 4, 4,
	 3, 5, 2, 6, 1, 7, 2, 7, 3, 6, 4, 5, 5, 4, 6, 3, 7, 2, 7, 3,
	 6, 4, 5, 5, 4, 6, 3, 7, 4, 7, 5, 6, 6, 5, 7, 4, 7, 5, 6, 6,
	 5, 7, 6, 7, 7, 6, 7, 7};

/*------------------------------- Compression JPEG ----------------------------------------*/
void QTinitialization(int Quality)
{
 double Qs;

 if (Quality <= 0)  Quality = 1;
 if (Quality > 100) Quality = 100;
 if (Quality < 50)   Qs = 50.0 / (double) Quality;
	else	     Qs = 2.0 - (double) Quality/50.0;


 // Calcul des coefficients de la table de quantification
 for (int u=0; u<8; u++)
   for (int v=0; v<8; v++)
	{
	 LuminanceJPEGTable[u][v] = OriginalLuminanceJPEGTable[u][v] * Qs;
	 if (LuminanceJPEGTable[u][v] < 1.0) LuminanceJPEGTable[u][v]=1.0;
	 if (LuminanceJPEGTable[u][v] > 255.0) LuminanceJPEGTable[u][v]=255.0;
	}

 return;
}


void JPEGdecoding(BMPImageStruct *InputImage , BMPImageStruct *OutputImage)
{
   double Block[8][8];
   double tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
   double tmp10, tmp11, tmp12, tmp13, tmp20, tmp23;
   double tmp;

   // Decodage bloc par bloc
   for (int i=0; i<InputImage->imageVsize; i=i+8)
	 for (int j=0; j<InputImage->imageHsize; j=j+8)
	{
	 for (int u=0; u<8; u++)
	   for (int v=0; v<8; v++) Block[u][v]=InputImage->data[i+u][j+v];

	// Quantification inverse
	for (int u=0; u<8; u++)
	   for (int v=0; v<8; v++)
	   Block[u][v] = Block[u][v] * LuminanceJPEGTable[u][v];

	// DCT inverse
	Block[0][0]+=1024.0;

	for (int u=0; u<8; u++)
	  {
	   // 1ère étape: Partie paire
	   tmp0=Block[u][0]+Block[u][4];
	   tmp1=Block[u][0]-Block[u][4];
	   tmp=(Block[u][2]+Block[u][6])*a23;
	   tmp2=tmp-(a2*Block[u][6]);
	   tmp3=tmp+(a3*Block[u][2]);

	   tmp10=tmp0+tmp3;
	   tmp13=tmp0-tmp3;
	   tmp11=tmp1+tmp2;
	   tmp12=tmp1-tmp2;

	   // 1ème étape: Partie impaire
	   tmp0=Block[u][1]-Block[u][7];
	   tmp3=Block[u][1]+Block[u][7];
	   tmp1=Block[u][3]*rc2;
	   tmp2=Block[u][5]*rc2;
	   tmp4=tmp0+tmp2;
	   tmp6=tmp0-tmp2;
	   tmp7=tmp3+tmp1;
	   tmp5=tmp3-tmp1;

	   // 2ème étape
	   tmp=(tmp4+tmp7)*a47;
	   tmp20=tmp-(a4*tmp7);
	   tmp23=tmp+(a7*tmp4);
	   Block[u][0]=tmp10+tmp23;
	   Block[u][7]=tmp10-tmp23;
	   Block[u][3]=tmp13+tmp20;
	   Block[u][4]=tmp13-tmp20;

	   tmp=(tmp5+tmp6)*a56;
	   tmp20=tmp-(a5*tmp6);
	   tmp23=tmp+(a6*tmp5);
	   Block[u][2]=tmp12+tmp20;
	   Block[u][5]=tmp12-tmp20;
	   Block[u][1]=tmp11+tmp23;
	   Block[u][6]=tmp11-tmp23;
	  }

	for (int v=0; v<8; v++)
	  {
	   // 1ère étape: Partie paire
	   tmp0=Block[0][v]+Block[4][v];
	   tmp1=Block[0][v]-Block[4][v];
	   tmp=(Block[2][v]+Block[6][v])*a23;
	   tmp2=tmp-(a2*Block[6][v]);
	   tmp3=tmp+(a3*Block[2][v]);

	   tmp10=tmp0+tmp3;
	   tmp13=tmp0-tmp3;
	   tmp11=tmp1+tmp2;
	   tmp12=tmp1-tmp2;

	   // 1ème étape: Partie impaire
	   tmp0=Block[1][v]-Block[7][v];
	   tmp3=Block[1][v]+Block[7][v];
	   tmp1=Block[3][v]*rc2;
	   tmp2=Block[5][v]*rc2;
	   tmp4=tmp0+tmp2;
	   tmp6=tmp0-tmp2;
	   tmp7=tmp3+tmp1;
	   tmp5=tmp3-tmp1;

	   // 2ème étape
	   tmp=(tmp4+tmp7)*a47;
	   tmp20=tmp-(a4*tmp7);
	   tmp23=tmp+(a7*tmp4);
	   Block[0][v]=tmp10+tmp23;
	   Block[7][v]=tmp10-tmp23;
	   Block[3][v]=tmp13+tmp20;
	   Block[4][v]=tmp13-tmp20;

	   tmp=(tmp5+tmp6)*a56;
	   tmp20=tmp-(a5*tmp6);
	   tmp23=tmp+(a6*tmp5);
	   Block[2][v]=tmp12+tmp20;
	   Block[5][v]=tmp12-tmp20;
	   Block[1][v]=tmp11+tmp23;
	   Block[6][v]=tmp11-tmp23;
	  }

	// On vérifie que cela ne déborde pas
	for (int u=0; u<8; u++)
	   for (int v=0; v<8; v++)
	{
	   Block[u][v]/=8.0;
	   if (Block[u][v] < 0.0) Block[u][v]=0.0;
	   if (Block[u][v] > 255.0) Block[u][v]=255.0;
	}

	// On range le résultat dans l'image de sortie
	 for (int u=0; u<8; u++)
	   for (int v=0; v<8; v++)
		OutputImage->data[i+u][j+v]=Block[u][v];
   }

   return;
}


int JPEGdepacketization(BMPImageStruct *OutputImage, FILE* TRACEFILE)
{
   int Block[8][8];
   unsigned int BlockOffset, row, col, row_mix, col_mix, packetsize, index, q, r, K;
   opj_mqc_t mqobjet;
   opj_mqc_t *objet=NULL;
   unsigned char buffer[MQC_NUMCTXS];

	// On lit la taille du prochain packet
	if (fscanf(TRACEFILE, "%4X", &packetsize) ==EOF) return 0;

	fscanf(TRACEFILE, "%2X %2X", &row, &col);
	BlockOffset = row * 256 + col;
	packetsize -= 2;

	for (int x=0; x<packetsize; x++) fscanf(TRACEFILE, "%2X", &buffer[x]);
	//packetcount++;
	objet=&mqobjet;
	mqc_init_dec(objet, buffer, packetsize);
	mqc_resetstates(objet);

   while (mqc_numbytes(objet) < packetsize) {
	// On décode
	q=0;
	while(mqc_decode(objet)==1) q++;
	r=mqc_decode(objet);
	K=q*2+r;
	for (int x=0; x<K; x++)
	{
	q=0;
	while(mqc_decode(objet)==1) q++;
	r=mqc_decode(objet);
	index=q*2+r;
	if ((index % 2) == 0)
		{  Block[ZigzagCoordinates[x].row][ZigzagCoordinates[x].col]=index / 2;	}
	   else {  Block[ZigzagCoordinates[x].row][ZigzagCoordinates[x].col]=0-((index+1) / 2);	}
	}
	for (int x=K; x<64; x++) Block[ZigzagCoordinates[x].row][ZigzagCoordinates[x].col]=0;
	// On copie le bloc décodé à sa place dans l'image
	row = (BlockOffset * 8) / OutputImage->imageHsize * 8;
	col = (BlockOffset * 8) % OutputImage->imageHsize;
	row_mix = ((row * 5) + (col *  8)) % (OutputImage->imageHsize);
	col_mix = ((row * 8) + (col * 13)) % (OutputImage->imageVsize);
	for (int u=0; u<8; u++)
	   for (int v=0; v<8; v++) OutputImage->data[row_mix+u][col_mix+v]=(double) Block[u][v];
	BlockOffset++;
   }

   if (BlockOffset != (OutputImage->imageHsize * OutputImage->imageVsize / 64)) {
	q=0;
	while((mqc_decode(objet)==1) && (q < 32)) q++;
	r=mqc_decode(objet);
	K=q*2+r;
	if (K > 64) return packetsize;
	for (int x=0; x<K; x++)
	{
	q=0;
	while((mqc_decode(objet)==1) && (q < 32)) q++;
	r=mqc_decode(objet);
	index=q*2+r;
	if ((index % 2) == 0)
		{  Block[ZigzagCoordinates[x].row][ZigzagCoordinates[x].col]=index / 2;	}
	   else {  Block[ZigzagCoordinates[x].row][ZigzagCoordinates[x].col]=0-((index+1) / 2);	}
	}
	for (int x=K; x<64; x++) Block[ZigzagCoordinates[x].row][ZigzagCoordinates[x].col]=0;
	// On copie le bloc décodé à sa place dans l'image
	row = (BlockOffset * 8) / OutputImage->imageHsize * 8;
	col = (BlockOffset * 8) % OutputImage->imageHsize;
	row_mix = ((row * 5) + (col *  8)) % (OutputImage->imageHsize);
	col_mix = ((row * 8) + (col * 13)) % (OutputImage->imageVsize);
	for (int u=0; u<8; u++)
	   for (int v=0; v<8; v++) OutputImage->data[row_mix+u][col_mix+v]=(double) Block[u][v];
	BlockOffset++;
   }

   return packetsize;
}

// end from CRAN

int WritePGMFile (char* FileName,  BMPImageStruct *Image)
{
	unsigned char pixel;
	FILE * ImageFile;
        unsigned int totalPixel=0;

	if ((ImageFile = fopen (FileName,"w")) == NULL) return -1;

	if (pgmBFile)
		fprintf(ImageFile, "P5\n");
	else
		fprintf(ImageFile, "P2\n");

	fprintf(ImageFile, "# test PGM\n");
	fprintf(ImageFile, "%d %d\n", Image->imageHsize, Image->imageVsize);
	fprintf(ImageFile, "255\n");

	for (int row=0; row<Image->imageVsize; row++)
		for (int col=0;col<Image->imageHsize;col++)
			{
			    pixel = (unsigned char) Image->data[row][col];

			    if (pgmBFile)
				fwrite(&pixel,1,1,ImageFile);
			    else
			    	fprintf(ImageFile, "%d ", (unsigned int)pixel);

			    totalPixel++;
			    
			    if (totalPixel % 12 == 0 && !pgmBFile)
				fprintf(ImageFile, "\n");
			}

	fclose(ImageFile);
	return 0;
}

void displayImage(char* image, char* captionString, bool waitKeyPress=false)
{
	SDL_Surface *hello = NULL;
	SDL_Surface *screen = NULL;
	SDL_Event event; /* Cette variable servira plus tard à gérer les évènements */

	bool nokeypress=true;

	// Start SDL
	SDL_Init( SDL_INIT_VIDEO );

	// Load image
        hello = IMG_Load (image);

	if (hello) { 
		// Set up screen
		// screen = SDL_SetVideoMode( hello->w, hello->h, 32, SDL_HWSURFACE );
		screen = SDL_SetVideoMode( 320, 320, 32, SDL_HWSURFACE );
		SDL_WM_SetCaption(captionString, NULL);
	 
		// Apply image to screen
		SDL_BlitSurface( hello, NULL, screen, NULL);

		// Update Screen
		SDL_Flip( screen );
	
		if (waitKeyPress && key_press_option) {
			while (nokeypress) { /* TANT QUE la variable ne vaut pas 0 */
				SDL_WaitEvent(&event); /* On attend un évènement qu'on récupère dans event */
				switch(event.type) /* On teste le type d'évènement */
				{
					case SDL_QUIT:
						nokeypress = false;
						break;
					case SDL_KEYDOWN:
						nokeypress= false;
						break;
				}
			}
		}
		else {
			// Pause 3s
			SDL_Delay(3000);
		}
		  
		// Free the loaded image
		SDL_FreeSurface( hello );
	}
	else
                printf("Cannot load image, probably because the file is corrupted.\n");

	// Quit SDL
	SDL_Quit();
}

void startDisplayImage(FILE* theFile, char* originalFile) {

    BMPImageStruct OriginalImage;
    char tmpBMPFile[80];
    int err;
    int psize, npkt=0, totalsize=0;

    // Read the original BMP file to get useful informations
    err = ReadBitmapFile(originalFile, &OriginalImage);

    if (err) {
            fprintf(stderr, "CANNOT read original BMP file %s\n", originalFile);

    }
    else {
            // reset content
            for (int i=0; i<OriginalImage.imageVsize; i++)
                    for (int j=0; j<OriginalImage.imageHsize; j++) OriginalImage.data[i][j]=0.0;

            // JPEG decoding
            while ((psize=JPEGdepacketization(&OriginalImage, theFile))) {
		totalsize+=psize;
  		npkt++;
	    }

            JPEGdecoding(&OriginalImage, &OriginalImage);

	    fprintf(stderr, "Encoded file size is %d, npkt is %d\n", totalsize, npkt);

            if (pgmFile) {
		sprintf(tmpBMPFile,"tmp_%d-%s.Q%d--P%d-S%d.pgm", fileSN, originalFile, qualityFactor, npkt, totalsize);
		WritePGMFile(tmpBMPFile, &OriginalImage);
	    }
	    
            sprintf(tmpBMPFile,"tmp_%d-%s.Q%d-P%d-S%d.bmp", fileSN, originalFile, qualityFactor, npkt, totalsize);

            err = WriteBitmapFile(tmpBMPFile, &OriginalImage, vflip);

            if (err) {
                    fprintf(stderr, "CANNOT write tmp BMP file from received encoded image.\n");
            }
            else {
                    displayImage(tmpBMPFile, "test", true);
                    //remove(tmpBMPFile);
            }

    }
}


void * printERROR(char *argv[])
{
   fprintf(stderr, "USAGE:\t%s -timer t -vflip -nokey -onlydisplay/-onlydisplay_r -pgm/-pgmb -pktd -pktf -framing -Q q orig_image_file_name\n", argv[0]);
   fprintf(stderr, "USAGE:\t-timer 5, set display timer to 5s\n");
   fprintf(stderr, "USAGE:\t-vflip, flip vertically the image\n");
   fprintf(stderr, "USAGE:\t-nokey, do not wait for key press\n");
   fprintf(stderr, "USAGE:\t-onlydisplay img_file.dat, only display the .dat file (produced by the encoder)\n");
   fprintf(stderr, "USAGE:\t-onlydisplay_r img_file.dat, only display the .dat file (previously received)\n");
   fprintf(stderr, "USAGE:\t-pgm/-pgmb, produce a PGM file (ASCII) or binary\n");
   fprintf(stderr, "USAGE:\t-pktd, display received frames\n");
   fprintf(stderr, "USAGE:\t-pktf, generate pkt list file\n");
   fprintf(stderr, "USAGE:\t-framing, expects 0xFF 0x55-0x59 for binary mode, 0xFF 0x50-0x54 for image mode, default is no framing\n");
   fprintf(stderr, "USAGE:\t-Q 40, use 40 as Quality Factor, default is 50\n");
   fprintf(stderr, "USAGE:\t orig_image_file_name, give the original bmp file\n");

   timespec start, stop, res;

   clock_getres(CLOCK_REALTIME, &res);

   int res1=clock_gettime(CLOCK_REALTIME, &start);
   usleep(15000);
   int res2=clock_gettime(CLOCK_REALTIME, &stop);

   fprintf(stderr, "debug info: %d %d %ld %ld %ld %ld %ld %ld %ld %.2f\n",
          res1, res2, res.tv_sec, res.tv_nsec, start.tv_sec, stop.tv_sec, start.tv_nsec, stop.tv_nsec,
          diff(start, stop).tv_sec, diff(start, stop).tv_nsec/1000000.0);

   return 0;
}

#ifdef THREAD_DISPLAY

void* threadDisplayImage(void *arg)
{
    FILE* displayFile;

    fprintf(stderr, "rcv pkt: %d\n", nbPktRcv);
    fprintf(stderr, "Quality Factor is : %d\n", qualityFactor);
    fprintf(stderr, "Opening file %s for display\n", rcvFile_tmp);

    // will read received image from this file
    displayFile = fopen(rcvFile_tmp, "r");

    startDisplayImage(displayFile, originalFile);

    fclose(displayFile);

    //printf("Deleting file %s\n", rcvFile);
    //remove(rcvFile);

    pthread_exit(NULL) ;
}

static void handler(int sig, siginfo_t *si, void *uc) {

        timespec now;

        fprintf(stderr, "Caught signal %d\n", sig);

        // delete timer
        timer_delete(timerid);

        clock_gettime(CLOCK_REALTIME, &now);

        fprintf(stderr, "Start display %lds%.2fms\n",
                diff(firstPkt, now).tv_sec,
                diff(firstPkt, now).tv_nsec/1000000.0);

        /////////////////////////////////////////////////////

        fclose(pFile);

        if (pktFile)
                fclose(dFile);

        /////////////////////////////////////////////////////

        if (nbPktRcv) {

            strcpy(rcvFile_tmp, rcvFile);

            int err = pthread_create(&(tid[0]), NULL, &threadDisplayImage, NULL);

            if (err != 0)
                fprintf(stderr, "\nCan't create thread: [%s]\n", strerror(err));
            else
                fprintf(stderr, "\nThread created successfully\n");
        }
        else
            fprintf(stderr, "no received pkt\n");

        // we collect stats per image
        nbPktRcv=0;

        // for naming temp file
        fileSN++;

        /////////////////////////////////////////////////////

        if (pktFile) {
                fprintf(stderr, "Creating file imageRcv.pktlist for storing the list of received packets\n");
                dFile = fopen("imageRcv.pktlist", "w");
        }
        else dFile=NULL;

        // will write received image pkt in this file
        sprintf(rcvFile,"tmp_%d-%s.Q%d.dat", fileSN, originalFile, qualityFactor);
        fprintf(stderr, "Creating file %s for storing the received image data file\n", rcvFile);

        pFile = fopen(rcvFile, "w");

        fprintf(stderr, "Wait for image\n");

        signal(sig, SIG_IGN);
}

#else

static void handler(int sig, siginfo_t *si, void *uc) {

	timespec now;

	fprintf(stderr, "Caught signal %d\n", sig);

	// delete timer
	timer_delete(timerid);

	clock_gettime(CLOCK_REALTIME, &now);

	fprintf(stderr, "Start display %lds%.2fms\n", 
		diff(firstPkt, now).tv_sec,
		diff(firstPkt, now).tv_nsec/1000000.0);

	/////////////////////////////////////////////////////

	fclose(pFile);

	if (pktFile)
		fclose(dFile);

	if (nbPktRcv) {
		fprintf(stderr, "rcv pkt: %d\n", nbPktRcv);
		fprintf(stderr, "Quality Factor is : %d\n", qualityFactor);
		fprintf(stderr, "Opening file %s for display\n", rcvFile);

		// will read received image from this file
		pFile = fopen(rcvFile, "r");

		startDisplayImage(pFile, originalFile);

		fclose(pFile);

		//printf("Deleting file %s\n", rcvFile);
		//remove(rcvFile);

		// we collect stats per image
		nbPktRcv=0;

		// for naming temp file
		fileSN++;
        }

	if (pktFile) {
		fprintf(stderr, "Creating file imageRcv.pktlist for storing the list of received packets\n");
		dFile = fopen("imageRcv.pktlist", "w");
	}
	else dFile=NULL;

	// will write received image pkt in this file
	sprintf(rcvFile,"tmp_%d-%s.Q%d.dat", fileSN, originalFile, qualityFactor);
	fprintf(stderr, "Creating file %s for storing the received image data file\n", rcvFile);

	pFile = fopen(rcvFile, "w");

	fprintf(stderr, "Wait for image\n");

        signal(sig, SIG_IGN);	   
}
#endif

int main(int argc, char *argv[]) {

    int i=0;
    bool onlyDisplay=false;

    if (argc < 2) {
       printERROR(argv);
       exit (-1);
    }

    for (int arg = 1; arg < argc-1; arg++) {

        if (!strcmp(argv[arg], "-vflip"))
        {
            vflip=true;
        }

        if (!strcmp(argv[arg], "-nokey"))
        {
            key_press_option=false;
        }

        if (!strcmp(argv[arg], "-onlydisplay"))
        {
            onlyDisplay=true;
	    display_received_dat=false;
            displayFile=argv[arg+1];
        }

        if (!strcmp(argv[arg], "-onlydisplay_r"))
        {
            onlyDisplay=true;
	    display_received_dat=true;
            displayFile=argv[arg+1];
        }

        if (!strcmp(argv[arg], "-pktf"))
        {
           pktFile=true;
        }

        if (!strcmp(argv[arg], "-pgm"))
        {
           pgmFile=true;
        }

        if (!strcmp(argv[arg], "-pgmb"))
        {
           pgmFile=true;
	   pgmBFile=true;
        }

        if (!strcmp(argv[arg], "-timer")) {
            display_timer_sec=atoi(argv[arg+1]);
        }

        if (!strcmp(argv[arg], "-Q")) {
            qualityFactor=atoi(argv[arg+1]);
        }

        if (!strcmp(argv[arg], "-pktd"))
        {
           pktDisplay=true;
        }

        if (!strcmp(argv[arg], "-framing"))
        {
           framing_mode=true;
           fprintf(stderr, "set to framing mode\n");
        }

    }

    // the file should always be the last argument
    originalFile=argv[argc-1];

    if (onlyDisplay) {

        pFile = fopen(displayFile, "r");

        unsigned int tmp;

	if (!display_received_dat) {
		
		// skip the field that are not usefull for display, just keep the quality Factor
		fscanf(pFile, "%04X", &tmp);
		fscanf(pFile, "%04X", &tmp);
		fscanf(pFile, "%04X", &tmp);
		fscanf(pFile, "%04X", &tmp);

		qualityFactor=tmp;
	}

        // Initialize the quantization matrix
        QTinitialization(qualityFactor);

        printf("Display image %s, original BMP file is %s, original QualityFactor is %d\n", displayFile, originalFile, qualityFactor);

        startDisplayImage(pFile, originalFile);

        fclose(pFile);
		exit(0);
    }

    // Initialize the quantization matrix
    QTinitialization(qualityFactor);

    fprintf(stderr, "Wait for image, original BMP file is %s, QualityFactor is %d\n", originalFile, qualityFactor);
    fprintf(stderr, "Display timer is %ds\n", display_timer_sec);
    // set the length of the real data
    myData_len=0;

    // allocate a new buffer to copy the real data, plus one char for the null-terminated character
    myData = (uint8_t*) calloc(120,sizeof(uint8_t));

    if (myData==NULL) {
		fprintf(stderr, "ERROR in getRawData:calloc:myData");
		return 0;
    }

    while (1) {

		uint8_t flag;

		if (pktFile) {
			fprintf(stderr, "Creating file imageRcv.pktlist for storing the list of received packets\n");
			dFile = fopen("imageRcv.pktlist", "w");
		}
		else dFile=NULL;

		// will write received image pkt in this file
		sprintf(rcvFile,"tmp_%d-%s.Q%d.dat", fileSN, originalFile, qualityFactor);
		fprintf(stderr, "Creating file %s for storing the received image data file\n", rcvFile);

		pFile = fopen(rcvFile, "w");

		fprintf(stderr, "Wait for image\n");

		/////////////////////////////////////////////////////

		while (1) {

			fread(&flag, sizeof(uint8_t), 1, stdin);

		  	if (flag==0xFF) {

		  		fread(&flag, sizeof(uint8_t), 1, stdin);
			
				// only image framing bytes	
			  	if (flag>=0x50 && flag<=0x54) {

					nbPktRcv++;

                                        // read the sequence number
                                        fread(&flag, sizeof(uint8_t), 1, stdin);

                                        // read the quality factor
                                        fread(&flag, sizeof(uint8_t), 1, stdin);

					if (flag!=qualityFactor) {
						qualityFactor=flag;
						QTinitialization(qualityFactor);
					}

					// read the packet size 
                                        fread(&myData_len, sizeof(uint8_t), 1, stdin);

					// read the data
					fread(myData, 1, myData_len, stdin);

					clock_gettime(CLOCK_REALTIME, &currentPkt);

					if (nbPktRcv==1) {

						clock_gettime(CLOCK_REALTIME, &firstPkt);
						    firstPkt=currentPkt;
						    lastPkt=currentPkt;

						/* Establish handler for timer signal */

						fprintf(stderr, "Establishing handler for signal %d\n", SIG);
						sa.sa_flags = SA_SIGINFO;
						sa.sa_sigaction = handler;
						sigemptyset(&sa.sa_mask);
						if (sigaction(SIG, &sa, NULL) == -1)
							errExit("sigaction");

						/* Block timer signal temporarily */

						fprintf(stderr, "Blocking signal %d\n", SIG);
						sigemptyset(&mask);
						sigaddset(&mask, SIG);
						if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
							errExit("sigprocmask");

						/* Create the timer */

						sev.sigev_notify = SIGEV_SIGNAL;
						sev.sigev_signo = SIG;
						sev.sigev_value.sival_ptr = &timerid;
						if (timer_create(CLOCKID, &sev, &timerid) == -1)
							errExit("timer_create");

						fprintf(stderr, "timer ID is 0x%lx\n", (long) timerid);

						/* Start the timer */
						// default is 5s
						freq_nanosecs = (long long)display_timer_sec*1000000000;
						its.it_value.tv_sec = freq_nanosecs / 1000000000;
						its.it_value.tv_nsec = freq_nanosecs % 1000000000;
						its.it_interval.tv_sec = its.it_value.tv_sec;
						its.it_interval.tv_nsec = its.it_value.tv_nsec;

						if (timer_settime(timerid, 0, &its, NULL) == -1)
							errExit("timer_settime");

						/* Unlock the timer signal, so that timer notification
						can be delivered */

						fprintf(stderr, "Unblocking signal %d\n", SIG);
						if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
							errExit("sigprocmask");

					} 

		            fprintf(stderr, "pkt %d(%ld %.2fms -> %lds%.2fms), SN %d, write %d bytes\n",
				                    nbPktRcv,
				                    currentPkt.tv_nsec,
				                    diff(lastPkt, currentPkt).tv_nsec/1000000.0,
				                    diff(firstPkt, currentPkt).tv_sec,
				                    diff(firstPkt, currentPkt).tv_nsec/1000000.0,
									flag,
				                    myData_len);

					lastPkt=currentPkt;
				
		    		// write the image chunk size
		    		fprintf(pFile, "%04X ", myData_len);

		    		if (pktDisplay)
		        		fprintf(stderr, "%.4X ", myData_len);

		    		if (pktFile)
		        		fprintf(dFile, "%04X (%d)", myData_len, myData_len);

		    		for (int i=0; i<myData_len; i++) {

		        		fprintf(pFile, "%02X ", myData[i]);

		        		if (pktDisplay)
		            		fprintf(stderr, "%.2X ", myData[i]);

		        		if (pktFile)
		            			fprintf(dFile, "%02X", myData[i]);

		        		if ( (i==0 || i==1) && pktFile)
		            			fprintf(dFile, " ");
		    		}

					fflush(pFile);

		    		fprintf(stderr, "\n");

		    		if (pktFile) {
		        		fprintf(dFile, "\n");	
						fflush(dFile);
					}
				}
			}
		}
    }
}
