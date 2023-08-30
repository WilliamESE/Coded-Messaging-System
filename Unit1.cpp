//---------------------------------------------------------------------------
#include <vcl.h>
#include <stdio.h>
#include <stdlib.h>
#pragma hdrstop

#include "Unit1.h"
#include "RxThreadlp.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "CGAUGES"
#pragma link "CSPIN"
#pragma link "PERFGRAP"
#pragma link "CGRID"
#pragma resource "*.dfm"

#define FILENAME "C:\\Users\\William\\Dropbox\\Conestoga\\Year_2\\Fall_2014\\C++Project\\SavesStuff.dat"

TForm1 *Form1;
//*****************************Linked list**********************************
typedef struct Message
	{
	char mess[140];
	int length;
	int senderID;
	int reciverID;
	int Priority;
    int Spec;
	struct Message *next;
	struct Message *prev;
	struct Message *Pinext;
	struct Message *Piprev;
	}messagetp;

typedef struct Recordings   //Will only allow 5 recordings
	{
    long Size;
    char *Data;
    int reciverID;
    struct Recordings *Next;
    }rectp;
rectp *RcHead;
rectp *RcTail;
int Rccnt;
int Limit = 0;
messagetp *Mshead;
messagetp *Mstail;
messagetp *Msmove;
int Mscnt;

//Priority
messagetp *Pihead[7];
messagetp *Pitail[7];
messagetp *Pimove[7];
int Picnt[7];

//*****************************Other Globals*******************************
int MyID;
DWORD Freq = 44100/8;   //Record sampling
DWORD Freq2 = 44100/8;   //Play back speed
int SubtractAm = 0;
char Run1 = 0;
char Run2 = 0;
HANDLE hCom;
bool ConnectState = false; // not connected on start-up.
char COMname[5];
int COMspeedV;
RxThread *rxthr = NULL;
unsigned char rbuf[100000];
int Mrec ,Msen, Mpri, Mtype, Mtyp;
int debugStuff = 5000;
char OldFindText[50];
char RawData[1000];
int RawIdx;
int NumMes, NumErr, Numdel;
int Writing = 1;
int cntGraph = 0;
long DataSum;
int freeze=0;
char CompressedR = 0;
char CompressedM = 0;
TColor grid;
bool Rec;
unsigned char Max[2], Min[2];
FILE *Fil;
struct Header
	{
    long lSignature;	// must be 0xDEADBEEF
	unsigned char bReceiverAddr;	// receiver ID. 0xff=broadcast
	unsigned char bVersion;		// must be 1 for version 1
	long lDataLength;	// size of message
	unsigned char bPattern[4];	// must be 0xaa 0x55 0xaa 0x55
	};

struct Header Head;
 //*************************Compresstion*******************************
typedef struct HuffTree
	{
	char type;
	int Value;
	unsigned char chara;
	struct HuffTree *Left;
	struct HuffTree *Right;
	}hufftp;

hufftp *root;
hufftp *move;
int cnText[256];
unsigned char Compress2[100000];
unsigned char Compress3[100000];
int SaveHuff[256][2];
int cntTrees = 0;
unsigned long Mlen;
unsigned long NumChar;
int Hold[2];
int run1=0;

/*********************************************************************\
* 		Message functions
\*********************************************************************/

void ShowProgress(char Test[])
    {
    Form1->recprg2->Top = (Form1->Height / 2) - (Form1->recprg2->Height / 2);
    Form1->recprg2->Left = (Form1->Width / 2) - (Form1->recprg2->Width / 2);
    Form1->Label12->Caption = Test;
    Form1->recprg2->Visible = true;
    }
void UpdateProgress(char Test[])
    {
    Form1->Label12->Caption = Test;
    Form1->recprg2->Update();
    }
/*********************************************************************\
* 		WaveHeader (WAVEHDR) wrapper functions
\*********************************************************************/
bool WaveMakeHeader(unsigned long ulSize, HGLOBAL &HData, HGLOBAL &HWaveHdr,LPSTR &lpData, LPWAVEHDR &lpWaveHdr)
    {
    //mallocs WAVEHDR structure
    HData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, ulSize);
    if (!HData) return false;

    lpData = (LPSTR)GlobalLock(HData);
    if (!lpData)
        {
        GlobalFree(HData);
        return false;
        }

    HWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, sizeof(WAVEHDR));
    if (!HWaveHdr)
        {
        GlobalUnlock(HData);
        GlobalFree(HData);
        return false;
        }

    lpWaveHdr = (LPWAVEHDR)GlobalLock(HWaveHdr);
    if (!lpWaveHdr)
        {
        GlobalUnlock(HWaveHdr);
        GlobalFree(HWaveHdr);
        GlobalUnlock(HData);
        GlobalFree(HData);
        return false;
        }

    ZeroMemory(lpWaveHdr, sizeof(WAVEHDR));
    lpWaveHdr->lpData = lpData;
    lpWaveHdr->dwBufferLength = ulSize;

	return true;
    }
//---------------------------------------------------------------------------

void WaveFreeHeader(HGLOBAL &HData, HGLOBAL &HWaveHdr)
    {
    GlobalUnlock(HWaveHdr);
    GlobalFree(HWaveHdr);
    GlobalUnlock(HData);
    GlobalFree(HData);
    }
//---------------------------------------------------------------------------

/*********************************************************************\
* 		WaveIn (recording) wrapper functions
\*********************************************************************/

bool WaveRecordOpen(LPHWAVEIN lphwi, HWND Hwnd, int nChannels,long lFrequency, int nBits)
    {
    WAVEFORMATEX wfx;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = (WORD)nChannels;
    wfx.nSamplesPerSec = (DWORD)Freq;
    wfx.wBitsPerSample = (WORD)nBits;
    wfx.nBlockAlign = (WORD)((wfx.nChannels * wfx.wBitsPerSample) / 8);
    wfx.nAvgBytesPerSec = (wfx.nSamplesPerSec * wfx.nBlockAlign);
    wfx.cbSize = 0;

    //finds recorder
    MMRESULT result = waveInOpen(lphwi, WAVE_MAPPER, &wfx, (LONG)Hwnd, NULL,
								 CALLBACK_WINDOW);

    if (result == MMSYSERR_NOERROR) return true;
	return false;
    }
//---------------------------------------------------------------------------

bool WaveRecordBegin(HWAVEIN hwi, LPWAVEHDR &lpWaveHdr)
    {
    MMRESULT result = waveInPrepareHeader(hwi, lpWaveHdr, sizeof(WAVEHDR));
    if (result == MMSYSERR_NOERROR)
        {
		MMRESULT result = waveInAddBuffer(hwi, lpWaveHdr, sizeof(WAVEHDR));
	    if (result == MMSYSERR_NOERROR)
            {
        	MMRESULT result = waveInStart(hwi);     //Starts recording
		    if (result == MMSYSERR_NOERROR) return true;
            }
        }
  	return false;    
    }
//---------------------------------------------------------------------------

void WaveRecordEnd(HWAVEIN hwi, LPWAVEHDR &lpWaveHdr)
    {
	waveInStop(hwi);
	waveInReset(hwi);
	waveInUnprepareHeader(hwi, lpWaveHdr, sizeof(WAVEHDR));    
    }
//---------------------------------------------------------------------------

void WaveRecordClose(HWAVEIN hwi)
    {
	waveInReset(hwi);
	waveInClose(hwi);
    }
//---------------------------------------------------------------------------

/*********************************************************************\
* 		WaveOut (playback) wrapper functions
\*********************************************************************/

bool WavePlayOpen(LPHWAVEOUT lphwo, HWND Hwnd, int nChannels,long lFrequency, int nBits)
    {
    WAVEFORMATEX wfx;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = (WORD)nChannels;
    wfx.nSamplesPerSec = (DWORD)Freq2;
    wfx.wBitsPerSample = (WORD)nBits;
    wfx.nBlockAlign = (WORD)((wfx.nChannels * wfx.wBitsPerSample) / 8);
    wfx.nAvgBytesPerSec = (wfx.nSamplesPerSec * wfx.nBlockAlign);
    wfx.cbSize = 0;

    MMRESULT result = waveOutOpen(lphwo, WAVE_MAPPER, &wfx, (LONG)Hwnd, NULL,
								  CALLBACK_WINDOW);

    if (result == MMSYSERR_NOERROR) return true;
	return false;
    }
//---------------------------------------------------------------------------

bool WavePlayBegin(HWAVEOUT hwo, LPWAVEHDR &lpWaveHdr)
    {
	MMRESULT result = waveOutPrepareHeader(hwo, lpWaveHdr, sizeof(WAVEHDR));
    if (result == MMSYSERR_NOERROR)
        {
		MMRESULT result = waveOutWrite(hwo, lpWaveHdr, sizeof(WAVEHDR));
    	if (result == MMSYSERR_NOERROR) return true;
        }
    return false;
    }
//---------------------------------------------------------------------------

void WavePlayEnd(HWAVEOUT hwo, LPWAVEHDR &lpWaveHdr)
    {
 	waveOutReset(hwo);
	waveOutUnprepareHeader(hwo, lpWaveHdr, sizeof(WAVEHDR));
    }
//---------------------------------------------------------------------------

void WavePlayClose(HWAVEOUT hwo)
    {
 	waveOutReset(hwo);
	waveOutClose(hwo);
    }
//---------------------------------------------------------------------------
/**************************************************************************\
Working with com port
\**************************************************************************/
void opencomm()
    {
    DWORD dwError;
    char buf[256];
    COMMTIMEOUTS CommTim;
    DCB dcb;
    bool fSuccess;
    Form1->Shape1->Brush->Color = clWhite;
    Form1->Shape2->Brush->Color = clWhite;
    hCom = CreateFile(COMname,
        GENERIC_READ | GENERIC_WRITE,
        0,    /* comm devices must be opened w/exclusive-access */
        NULL, /* no security attrs */
        OPEN_EXISTING, /* comm devices must use OPEN_EXISTING */
        0,    /* non overlapped I/O */
        NULL  /* hTemplate must be NULL for comm devices */
        );
    //real termnial
    if (hCom == INVALID_HANDLE_VALUE)
        {
        dwError = GetLastError();
        sprintf(buf,"Opened, dwError = %d",dwError);
        Form1->ckbcomopen->Checked = false;
        Form1->CheckBox3->Checked = false;
        }
    else
        {
        sprintf(buf,"Opened");
        Form1->ckbcomopen->Caption = buf;
        Form1->ckbcomopen->Checked = true;
        Form1->CheckBox3->Checked = true;
        }
    Form1->CheckBox3->Caption = buf;
    Form1->ckbcomopen->Caption = buf;
    //result = ReadFile(hCom,trbuf,1,&nread,NULL);
    fSuccess = GetCommState(hCom, &dcb);
    if (!fSuccess)
        {
        dwError = GetLastError();
        sprintf(buf,"State Found, dwError = %d",dwError);
        Form1->ckbCOMState->Checked = false;
        Form1->CheckBox4->Checked = false;
        }
    else
        {
        sprintf(buf,"State Found");
        Form1->ckbCOMState->Checked = true;
        Form1->CheckBox4->Checked = true;
        }
    Form1->ckbCOMState->Caption = buf;
    Form1->CheckBox4->Caption = buf;
    if (Form1->PageSen->Visible)
        COMspeedV = Form1->COMspeed->Value;
    else
        COMspeedV = Form1->CSpinEdit2->Value;
    dcb.BaudRate = COMspeedV;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = true;
    dcb.fOutxCtsFlow = false;

    Form1->COMspeed->Value = COMspeedV;
    Form1->CSpinEdit2->Value = COMspeedV;
//  dcb.fOutxDsrFlow = false;
    //dcb.fDtrControl = DTR_CONTROL_ENABLE; // This supplies power for
        // the Opto Interface used for the controller!  DISABLE won't
        // work very well.
    dcb.fDsrSensitivity = false;
    dcb.fOutX = false;
    dcb.fInX = false;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    fSuccess = SetCommState(hCom, &dcb);
    if (!fSuccess)
        {
        dwError = GetLastError();
        sprintf(buf,"State Writeable, dwError = %d",dwError);
        Form1->ckbCOMSet->Checked = false;
        Form1->CheckBox5->Checked = false;
        }
    else
        {
        sprintf(buf,"State Writeable");
        Form1->ckbCOMSet->Checked = true;
        Form1->CheckBox5->Checked = true;
        }
    Form1->ckbCOMSet->Caption = buf;
    Form1->CheckBox5->Caption = buf;
    fSuccess = GetCommTimeouts(hCom,&CommTim); // Read Communications Timeouts
    if (!fSuccess)
        {
        dwError = GetLastError();
        sprintf(buf,"Time-out found, dwError = %d",dwError);
        Form1->ckbCOMTime->Caption = buf;
        Form1->CheckBox6->Caption = buf;
        Form1->ckbCOMTime->Checked = false;
        Form1->ckbCOMTimeWrite->Checked = false;
        Form1->CheckBox6->Checked = false;
        Form1->CheckBox7->Checked = false;
        }
    else
        {
        sprintf(buf,"Time-out found");
        Form1->ckbCOMTime->Caption = buf;
        Form1->ckbCOMTime->Checked = true;
        Form1->CheckBox6->Checked = true;
        Form1->CheckBox6->Caption = buf;
        CommTim.ReadTotalTimeoutMultiplier = 3; //
        CommTim.ReadTotalTimeoutConstant = 250l;
        fSuccess = SetCommTimeouts(hCom,&CommTim);
        if (!fSuccess)
        	{
            dwError = GetLastError();
            sprintf(buf,"Time-out Writeable, dwError = %d",dwError);
            Form1->ckbCOMTimeWrite->Caption = buf;
            Form1->CheckBox7->Caption = buf;
            Form1->ckbCOMTimeWrite->Checked = false;
            Form1->CheckBox7->Checked = false;
            }
        else
            {
            sprintf(buf,"Time-out Writeable");
            Form1->ckbCOMTimeWrite->Caption = buf;
            Form1->CheckBox7->Caption = buf;
            Form1->ckbCOMTimeWrite->Checked = true;
            Form1->CheckBox7->Checked = true;
            }
        //rval = true;
        ConnectState = true; // Indicate we are now connected.
        Form1->Shape1->Brush->Color = clRed;
        Form1->Shape2->Brush->Color = clRed;
        }
    }

void closecomm()
    { // Close and release the Comm port.
    if (ConnectState)
        {
        CloseHandle(hCom); // close this channel
        ConnectState = false;
        }
    }
/***************************************************************************
Linked list functions:
int         MesInit()               -Mallocs Head and sets the tail
                                    -Return 1 for success and 0 for fail
messagetp   FinMess(int indx)       -Looks through the list and find the
                                        items at indx
                                    -returns a pointer to the item or NULL
                                        if nothing was found
int         NewMess(int indx)       -Adds a new items anywhere in the list
                                    -returns 0 for failure and 1 for success
int         DelMess(int indx)       -Deletes an items anywhere from the list
                                    -returns 0 for failure and 1 for success
***************************************************************************/
int MesInit()
	{
	int x;
	//Setup head
	Mshead = (messagetp *)malloc(sizeof(messagetp));
	if (!Mshead) return 0;
	Mstail = Mshead;
	Mscnt = 1;
	//Init values
	for(x=0;x<7;x++)
		{
		Pihead[x] = NULL;
		Pitail[x] = NULL;
		Picnt[x] = 0;
		}
	return 1;
	}
//---------------------------------------------------------------------------
messagetp *FinMess(int indx)
	{
	int Half = Mscnt / 2;
	int x;
	messagetp *Found = NULL;
    messagetp *Old = NULL;
	if (indx > Half)
		{
		Old = Mstail;
		//Start at back
		for (x=(Mscnt-1); x>=0; x--)
			{
			if (x == indx)
				Found = Old;
			Old = Old->prev;
			}
		}
	else
		{
		Old = Mshead;
		//start at beginning
		for (x=0; x<Mscnt; x++)
			{
			if (x == indx)
				Found = Old;
			Old = Old->next;
			}
		}
	return Found;
	}

int FinIndex(messagetp *Found)
	{
	int x;
    int fnd;
    messagetp *Old = NULL;
    Old = Mstail;
    for (x=(Mscnt-1); x>=0; x--)
        {
        if (Found->Spec == Old->Spec)
            fnd = x;
        Old = Old->prev;
        }
	return fnd;
	}
//---------------------------------------------------------------------------
messagetp *NewMess(int indx)
	{
	messagetp *Newmess;
    messagetp *Oldmess;
    //Find the item at the location
	Oldmess = FinMess(indx);

    //Acconts for inserting at bottom of list
	if (Oldmess == NULL)	Oldmess = FinMess(indx-1);

	Newmess = (messagetp *)malloc (sizeof(messagetp));
	if (!Newmess) return NULL;

	if (indx < Mscnt)
		Newmess->next = Oldmess;

	else if ((indx > 0) && (indx != Mscnt))
		{
		Oldmess->prev->next = Newmess;
		Newmess->prev = Oldmess->prev;
		}
	else if ((indx == Mscnt) && (Mscnt != 0))
		{
		Newmess->prev = Oldmess;
		Oldmess->next = Newmess;
		}
    //else if ((indx == Mscnt) && (Mscnt == 0))
	  //	{

        //}
	else
		Mshead = Newmess;

	if (indx < Mscnt)
		Oldmess->prev = Newmess;
	else
		Mstail = Newmess;

	Mscnt++;
	return Newmess;
	}
//---------------------------------------------------------------------------
int DelPri(int indx, int Priotiy)
	{
	if (Picnt[Priotiy] > 0)
		{
		messagetp *Newmess;
        messagetp *Move1 = NULL;
		Move1 = FinMess(indx);
        if (Move1)
            {
    		if (indx <= 0)
	    		{
                if (Picnt[Priotiy] >= 2)
                	{
		    		Pihead[Priotiy] = Move1->Pinext;
			    	Move1->Pinext->Piprev = NULL;
                	}
                else
                	{
                    Pihead[Priotiy] = NULL;
                    }
			    }
    		else if (indx >= (Picnt[Priotiy]-1))
	    		{
		    	Pitail[Priotiy] = Move1->Piprev;
			    Move1->Piprev->Pinext = NULL;
    			}
	    	else
		    	{
			    Move1->Piprev->Pinext = Move1->Pinext;
    			Move1->Pinext->Piprev = Move1->Piprev;
	    		}
		    Move1->Pinext = NULL;
		    Move1->Piprev = NULL;
		    Picnt[Priotiy]--;
		    return 1;
            }
        else
            return 1;
		}
	else
		return 1;
	}


int DelMess(int indx)
	{
	messagetp *Newmess;
	Msmove = FinMess(indx);
    DelPri(indx,Msmove->Priority);
	if (indx <= 0)
		{
        if (Mscnt >= 2)
            {
			Mshead = Msmove->next;
			Msmove->next->prev = NULL;
        	}
        else
           Mshead = NULL;
		}
	else if (indx >= (Mscnt-1))
		{
		Mstail = Msmove->prev;
		Msmove->prev->next = NULL;
		}
	else
		{
		Msmove->prev->next = Msmove->next;
		Msmove->next->prev = Msmove->prev;
		}
	Msmove->next = NULL;
	Msmove->prev = NULL;
	free(Msmove);
	Mscnt--;
	return 1;
	}

int RcAdd(rectp *Node)
	{
    char buff[100];
    if ((RcHead == NULL) || (Rccnt >= 5))
    	{
        RcHead = Node;
    	RcTail = Node;
        Node->Next = NULL;
        if (Rccnt >= 5)
        	{
            Limit = 1;
        	Rccnt = 0;
            }
        Rccnt++;
        }
    else
    	{
        RcTail->Next = Node;
    	RcTail = Node;
        Node->Next = NULL;
        Rccnt++;
    	}
    if (Limit)
    	Form1->CheckListBox1->Items->Delete(0);
    sprintf(buff,"From: %i at: ",Node->reciverID);
    Form1->CheckListBox1->Items->Add(buff);
    }

void PrintMess(int From, int To)
    {
    int idx;
    char Buffer[1000];
    if (From <= To)
        {
        messagetp *ToMember;
        if (From == 0)
            Msmove=Mshead;
        else
            Msmove = FinMess(From);
        ToMember = FinMess(To);
        idx=From;
        for (Msmove=Msmove;Msmove!=ToMember->next;Msmove=Msmove->next)
            {
            sprintf(Buffer,"%s\r\nMessage #%i From ID: %i To ID %i Priority is: %i\r\n%s\r\n",
                Form1->rtbDisplay->Text,idx,Msmove->senderID,Msmove->reciverID,Msmove->Priority,Msmove->mess);
            Form1->rtbDisplay->Text = Buffer;
            idx++;
            }
        }
    }
void PrintPrio(int pri)
    {
    char Buffer[1000];
    int idx;
    if (Pihead[pri] != NULL)
        {
        idx = FinIndex(Pihead[pri]);
        for (Pimove[pri] = Pihead[pri];Pimove[pri]!=Pitail[pri]->Pinext;Pimove[pri]=Pimove[pri]->Pinext)
            {
            idx = FinIndex(Pimove[pri]);
            sprintf(Buffer,"%s\r\nMessage #%i From ID: %i To ID %i Priority is: %i\r\n%s\r\n",
                Form1->rtbDisplay->Text,idx,Pimove[pri]->senderID,Pimove[pri]->reciverID,Pimove[pri]->Priority,Pimove[pri]->mess);
            Form1->rtbDisplay->Text = Buffer;
            }
        }
    }
void PrintMessCombo()
    {
    int indx = Form1->cbxView->ItemIndex;
    int Pri;
    int x;
    char buff[255], buff2[32];
    if ((indx != 10) && (freeze == 0))
    	Form1->rtbDisplay->Text = " ";
    if (indx != -1)
        {
        if (indx == 0) //Show all
            {
            PrintMess(0,Mscnt-1);
            Form1->pnlViewFrom->Visible = false;
            }
        else if ((indx >= 1) && (indx <= 7)) //Priority
            {
            Pri = indx - 1;
            PrintPrio(Pri);
            Form1->pnlViewFrom->Visible = false;
            }
        else if (indx == 8)  //From x to y
            {
            PrintMess(Form1->nudFrom->Value, Form1->nudTo->Value);
            Form1->pnlViewFrom->Visible = true;
            }
        else if (indx == 9)  //Priority From - to
            {
            for(Pri=Form1->nudFrom->Value;Pri<=Form1->nudTo->Value;Pri++)
                PrintPrio(Pri);
            Form1->pnlViewFrom->Visible = true;
            Form1->nudTo->MaxValue = 6;
            }
        else if (indx == 10)  //Priority To - From
            {
            for(Pri=Form1->nudTo->Value;Pri>=Form1->nudFrom->Value;Pri--)
                PrintPrio(Pri);
            Form1->pnlViewFrom->Visible = true;
            Form1->nudTo->MaxValue = 6;
            }
        else if ((indx == 11) && (freeze == 0))
        	{
            if (Form1->ComboBox2->ItemIndex == -1)
            	Form1->ComboBox2->ItemIndex = 0;
            for(Pri=0;Pri<1000;Pri+=(Form1->CSpinEdit3->Value))
            	{
                strcpy(buff,"");
                for (x=Pri;(x<(Pri+(Form1->CSpinEdit3->Value))) && (x<1000);x++)
                	{
                    if (Form1->ComboBox2->ItemIndex == 0)
                       sprintf(buff2,"%c ",(unsigned char)RawData[x]);
                    else if (Form1->ComboBox2->ItemIndex == 1)
                	   sprintf(buff2,"%i ",(unsigned int)RawData[x]);
					else if (Form1->ComboBox2->ItemIndex == 2)
                       sprintf(buff2,"%02X ",(unsigned char)RawData[x]);
                    strcat(buff,buff2);
                	}
            	Form1->rtbDisplay->Lines->Add((AnsiString)buff);
                }
            }
        }
    Form1->rtbDisplay->SelectAll();
    Form1->rtbDisplay->SelAttributes->Color = clBlack;
    Form1->rtbDisplay->SelLength = 0;
    }



void StartThread()
	{


    }
/***************************************************************************
Linked list functions:
int    NewPri(int indx, int Priotiy)    -Adds an item to the bottom of the list
                                        -returns 1
int    DelPri(int indx, int Priotiy)    -Removes an item from the list
***************************************************************************/
int NewPri(int indx, int Priotiy)
	{
	messagetp *Newmess;
	Msmove = FinMess(indx);
	//Check to make sure there are values in heads and tails
	if (Pitail[Priotiy] == NULL) Pitail[Priotiy] = Msmove;
	else Pitail[Priotiy]->Pinext = Msmove;
	if (Pihead[Priotiy] == NULL) Pihead[Priotiy] = Msmove;

	Msmove->Piprev = Pitail[Priotiy];
	Pitail[Priotiy] = Msmove;
	Picnt[Priotiy]++;
	return 1;
	}

//---------------------------------------------------------------------------


int ChangePri(int indx, int newValue)
    {
    int checks;
    messagetp *pntr;
    pntr = FinMess(indx);
    if ((pntr->Priority >= 0) && (pntr->Priority <= 6))
        checks = DelPri(indx, pntr->Priority);
    else
        checks = 1;
    if (checks)
        checks = NewPri(indx, newValue);
    if (checks) pntr->Priority = newValue;
    return checks;
    }

void __fastcall RxThread::AddItReciver()
    {
    char buff[50];
    int x;
    Mtyp = 0;
    if (Mtyp)
    	{
   		if (Run2 == 1)
        	Msmove = NewMess(Mscnt);
    	else
        	{
        	Msmove = Mshead;
        	Run2 = 1;
        	}
    	sprintf(Msmove->mess,"%s",rbuf);
    	Msmove->length = strlen(Msmove->mess);
    	Msmove->senderID = Msen;
    	Msmove->Spec = Mscnt-1;
    	Msmove->Priority = 7;
    	ChangePri(Mscnt-1,Mpri);
    	Msmove->reciverID = Mrec;

    	PrintMessCombo();
	    Form1->nudTo->MaxValue = Mscnt-1;
    	NumMes++;
	    sprintf(buff,"%i Messages have been recieved",NumMes);
    	Form1->lblMessTotal->Caption = buff;

	    Form1->btnDeleteM->Enabled = true;
    	Form1->BitBtn11->Enabled = true;

	    sprintf(buff,"Message #%i: From %i, To %i, Priority %i",Mscnt-1,Msen,Mrec,Mpri);
    	Form1->RecMesBox->Items->Add(buff);
        }
    else
    	{
         Form1->Button5Click(NULL);
        }
    }

void FindText(char Text[50], TColor col)
    {
    int FoundAt, StartPos,ToEnd;
    TSearchTypes tp;
    FoundAt = 0;
    Form1->rtbDisplay->SelectAll();
    Form1->rtbDisplay->SelAttributes->Color = clBlack;
    Form1->rtbDisplay->SelLength = 0;
    while (FoundAt != -1)
        {
        if (Form1->rtbDisplay->SelLength)
            StartPos = Form1->rtbDisplay->SelStart + Form1->rtbDisplay->SelLength;
        else
            StartPos = 0;

        ToEnd = Form1->rtbDisplay->Text.Length() - StartPos;
        if (Form1->fdg->Options.Contains(frMatchCase))
            tp << stMatchCase;
        if (Form1->fdg->Options.Contains(frWholeWord))
            tp << stWholeWord;

        FoundAt = Form1->rtbDisplay->FindText(Form1->fdg->FindText, StartPos, ToEnd,tp);
        if (FoundAt != -1)
            {
            Form1->rtbDisplay->SelStart = FoundAt;
            Form1->rtbDisplay->SelLength = Form1->fdg->FindText.Length();
            Form1->rtbDisplay->SelAttributes->Color = col;
            }
        }

    }
/******************************************************************************
Compresstion Functions: RLE and huffman functions
******************************************************************************/
//FindSmallest, Takes an array of numbers and returns the index with the smallest
int FindSmallest(int Arr[], int Size, int cnt0)
	{
	int Lowest;
	int x;
	int LowIndx = -1;
	if (!cnt0)
		Lowest = Arr[0];
	else
		Lowest = 999999;     //Set the starting point greater then any number the function could run into
	for (x=0;x<Size;x++)
		{
		if ((Arr[x] < Lowest) && (Arr[x] != -1))
			{
			if (Arr[x] == 0)
				{
				if (!cnt0) //Check if 0's count
					{
					Lowest = Arr[x];
					LowIndx = x;
					}
				}
			else
				{
				Lowest = Arr[x];
				LowIndx = x;
				}
			}
		}

	return LowIndx;
	}
//---------------------------------------------------------------------------
//***************Takes 2 trees and connects them together**********************
hufftp *Combian(hufftp *node1, hufftp *node2)
	{
	hufftp *root1 = (hufftp *)malloc(sizeof(hufftp)); //create new tree
	if (!root1) return 0; //Check malloc
	root1->type = 1; //Set type to node
	//Add 2 values for the new node
	root1->Value = node1->Value + node2->Value;

	//Make sure the smaller node ends up on the left
	if (node1->Value > node2->Value)
		{
		root1->Left = node2;
		root1->Right = node1;
		}
	else
		{
		root1->Right = node2;
		root1->Left = node1;
		}
	return root1;
	}

//---------------------------------------------------------------------------
//*Search through the tree find a node then create an array of the 1's and 0's*
int GetValue(hufftp *Find, hufftp *Start,unsigned short out)
	{
	int Value1=0;
	//Type makes sure it doesn't mistake garbage for a character
	if ((Start->type == 1) && (run1 == 0))
		{
		out = GetValue(Find, Start->Right, out);
		if (run1 > 0) //Collect 1
			{
			out = out | (1 * (short)pow(2.0,15-(run1-1)));
			run1++;
			}
		}
	if ((Start->type == 1) && (run1 == 0))
		{
		out = GetValue(Find, Start->Left, out);
		if (run1 > 0) //Collect 0
			{
			//out = out | (0 * (short)pow(2.0,15-run1));
			run1++;
			}
		}
	if (Find == Start)
		{
		run1 = 1;
		return out;
		}
	return out;
	}

//---------------------------------------------------------------------------
//*****************Search Tree for a character and return that node************
hufftp *Search(unsigned char find, hufftp *Start, hufftp *Found)
	{
	if ((Start->chara == find) && (Start->type == 0))
		{
		Found = Start;
		return Found;
		}
	if (Start->type == 1)
		Found = Search(find, Start->Right,Found);
	if (Start->type == 1)
		Found = Search(find, Start->Left,Found);
	return Found;
	}

//---------------------------------------------------------------------------
/******************************************************************************
DeComHuffman takes huffman code and decompiles it into normal characters
In[] is the huffman code
int size of the huffman code
out[] is the output
Start is the root of the tree used to make the huffman code
******************************************************************************/
void __fastcall RxThread::DeComHuffman()
	{
	hufftp *Mover = root;
	int x,y,z;
	int Val;
	z=0;
	for (y=0x80,x=0;x<=(Mlen-7);y/=2)
		{
		Val = rbuf[x] & y;
        if (Mover != NULL)
            {
		    if (Val == 0)
			    Mover = Mover->Left;
    		else
	    		Mover = Mover->Right;
            if (Mover != NULL)
		    	if (Mover->type == 0) //Print out new character
			    	{
	    			Compress3[z] = Mover->chara;
		    		z++;
			    	Mover = root;
				    }
            }
        else
            {
            Form1->Panel2->Visible = false;
            ShowProgress("The program failed to recreate the tree!");
            Form1->grpah->Enabled = true;
            }
		if (y == 1) //Move to next character
			{
			y = 256;
			x++;
			}
		}
    Hold[1] = z;
	Compress3[z-1] = '\0'; //End string
	}

//---------------------------------------------------------------------------
/******************************************************************************
ComHuffman takes the string and compresses by huffman code

******************************************************************************/
void ComHuffman(unsigned char *Text, int size, unsigned char out[], int *SizeOut)
	{
	int x;
	int y;
	int z;
	int smidx;
	int smidx2;
	int smidx3;
	int NumBits;
    char CntChars;
	unsigned short Bits;
	int savetochar;
    int savetoShort;
	int BitValue;
	unsigned short Bits1;
	unsigned char UnCom[200];
	hufftp *Trees[256];
	hufftp *RememberMe[5];
	//Initalizes the counting array
	for (x=0;x<256;x++)
    	{
		cnText[x] = 0;
        SaveHuff[x][0] = 0;
        SaveHuff[x][1] = 0;
        }
	//Create trees and count how often a character appears
	for (x=0;x<size;x++)
		{
		if (cnText[Text[x]] == 0)
			{
			Trees[Text[x]] = (hufftp *)malloc(sizeof(hufftp));
			Trees[Text[x]]->chara = Text[x];
			Trees[Text[x]]->type = 0;
			Trees[Text[x]]->Value = 0;
			cntTrees++;
			}
		//Counts occerences
		cnText[Text[x]]++;
		Trees[Text[x]]->Value++;
		}

	//Combains the trees until there is only one left
	for(;cntTrees!=1;)
		{
		if (cntTrees == 4)
			x=0;
		smidx = FindSmallest(cnText, 256, 1);
		smidx3 = cnText[smidx];
		cnText[smidx] = -1;
		smidx2 = FindSmallest(cnText, 256, 1);
		if (smidx2 != -1)
			{			
			Trees[smidx2] = Combian(Trees[smidx], Trees[smidx2]);
			cnText[smidx2] = Trees[smidx2]->Value;
			root = Trees[smidx2]; //Saves the root
			}
		cnText[smidx] = -1;
		cntTrees--;
		}

	z=0;
	run1 = 0;
	savetochar = 128;
	out[0] = '\0';
	//Saves the 0's and 1's into char variables
	for (x=0;x<=size;x++)
		{
		run1 = 0;
		//Gets the bits into an array of chars
        Bits1 = 0;
		Bits = GetValue(Search(Text[x],root,NULL),root,Bits1);
		//The nuber of bits
		NumBits = (run1-2);
        SaveHuff[Text[x]][0] = run1-1;
        SaveHuff[Text[x]][1] = Bits;
        savetoShort = (int)pow(2,15-NumBits);
		for(y=NumBits;y>=0;y--)
			{
			//The 1's and 0's and combain them into 1 char
            if ((Bits & savetoShort) != 0)
            	CntChars = 1;
            else
            	CntChars = 0;
			out[z] = out[z] | (CntChars * savetochar);
            //SaveHuff[Text[x]][1] = SaveHuff[Text[x]][1] | (Bits1[y] * savetochar);
			savetochar /= 2;
            savetoShort *= 2;
			if (savetochar < 1)
				{
				savetochar = 128;
				z++;
				out[z] = '\0';
				}
			}
		}
	*SizeOut = z;
	}



void SendTree(hufftp *Start)
	{
    unsigned long nwrit;
    unsigned char Bits1[130];
    hufftp *Found;
    int x;
    int y;
    int savetochar;
    int SendByte = sizeof(int);
    int SendInt;
    //
    for(x=0;x<256;x++)
    	{
        if (SaveHuff[x][0] > 0)
        	{
            WriteFile(hCom,&x,1,&nwrit,NULL);
            WriteFile(hCom,&SaveHuff[x][0],1,&nwrit,NULL);
            WriteFile(hCom,&SaveHuff[x][1],sizeof(short),&nwrit,NULL);
            }
    	}
    }
void CollectTree(hufftp *Start)
	{
    DWORD nrd;
    while (nrd != 1)
    	ReadFile(hCom,&Start->type,1,&nrd,NULL);
    nrd = 0;
    while (nrd != 1)
    	ReadFile(hCom,&Start->Value,sizeof(int),&nrd,NULL);
    nrd = 0;
    while (nrd != 1)
    	ReadFile(hCom,&Start->chara,1,&nrd,NULL);
    if (Start->type == 1)
    	{
        Start->Left = (hufftp *)malloc(sizeof(hufftp));
    	if (!Start) return;
        CollectTree(Start->Left);
        Start->Right = (hufftp *)malloc(sizeof(hufftp));
    	if (!Start) return;
        CollectTree(Start->Left);
        }
    }

void __fastcall RxThread::StartSendOrRec()
	{
    root = (hufftp *)malloc(sizeof(hufftp));
    CollectTree(root);
    }
//---------------------------------------------------------------------------
void Compress(unsigned char Input[], int Size,unsigned char Output[],int ExtrInf[])
	{
	int x;
	unsigned char ESCChar;
	int Chars[256];
	int CharCnt=0;
	unsigned char CharValue=0;
	int idx=0;
	int holdValue;
	for(x=0;x<256;x++)
		Chars[x] = 0;
	//Print data and count the number of each type
	for(x=0;x<(Size);x+=1)
		Chars[*(Input+x)]++;

	//Find shortest value
	ESCChar = FindSmallest(Chars, 256, 0);

	for (x=0;x<(Size+1);x++)
		{
		if (x==Size - 10)
			x=x;
		if ((*(Input+x) == CharValue) && (CharCnt < 254))
			CharCnt++;
		else
			{
			if (CharCnt >= 4)
				{
				Output[idx] = ESCChar;
				Output[idx+1] = (CharCnt+1);
				Output[idx+2] = CharValue;
				idx+=3;
				}
			else
				{
				holdValue = idx;
				for(idx=idx;idx<((CharCnt+1) + holdValue);idx++)
                    {
					Output[idx] = CharValue;
                    }
                Form1->Label1->Caption = holdValue;
				}
			CharValue = *(Input+x);
			CharCnt = 0;
			}
		}
	ExtrInf[0] = ESCChar;
	ExtrInf[1] = idx;
	}

//---------------------------------------------------------------------------


/***************************************************************************
Startup functions
***************************************************************************/
__fastcall TForm1::TForm1(TComponent* Owner): TForm(Owner)
    {
    char Buffer[100];
    unsigned long nwrit;
    //Initate values
    ulSize = 100000;
	FAllocated = false;
    FRecording = false;
    FPlaying = false;

    TrackBar1->Max = 0xFFFF;
    TrackBar1->Frequency = 1000;

    DWORD current_volume;
    waveOutGetVolume(0, &current_volume);
    TrackBar1->Position = TrackBar1->Max - LOWORD(current_volume);

    sprintf(Buffer,"Recording Length\n%i bytes",ulSize);
    lblRecLen->Caption = Buffer;

    if (!MesInit())
        {
        sprintf(Buffer,"Unable to create message list");
        MessageBox(NULL,Buffer,"",MB_OK);
        gpbMessage->Visible = false;
        rtbDisplay->Visible = false;
        }
    sprintf(COMname,"COM7");
    COMNum->Value = 7;
    COMspeed->Value = 57600;
    COMspeedV = 57600;
    opencomm();

    rtbDisplay->Text = " ";
    Msmove = Mshead;

    rtbDisplay->Text = "No messages to display";


    //recprg
    }
//---------------------------------------------------------------------------
__fastcall TForm1::~TForm1()
    {
    //initate record and playback stuff
	if (FAllocated)	WaveFreeHeader(HData, HWaveHdr);
    if (FRecording) WaveRecordClose(hwi);
    if (FPlaying) WavePlayClose(hwo);

    }
/***************************************************************************
End of recording and playback functions
***************************************************************************/
//---------------------------------------------------------------------------
void __fastcall TForm1::MMWimData(TMessage &Msg)
    {
    int x,y;
    char buf[255],buf2[32];
    //Close the recording
	if (FRecording) WaveRecordEnd(hwi, lpWaveHdr);
    WaveRecordClose(hwi);

    //Enable Record,Play,Trackbar2 Disable Stop,Timer
    tmrRec->Enabled = false;
    RecordButton->Enabled = true;
    Button3->Enabled = true;
    TrackBar2->Enabled = true;
    rtbDisplay->Text = " ";

    Writing = 0;
    Max[0] = 0x80;
    Min[0] = 0x00;
    DataSum = 0;
    pgr->Progress = 0;
    UpdateProgress("Loading Info");
    recprg2->Repaint();
    DataSum = 0;
    for(x=0;x<ulSize;x+=32)
    	{
       	strcpy(buf,"");
        for(y=x;y<x+32;y++)
        	{
            sprintf(buf2,"%02X ",(unsigned char)*(lpData+y));
            DataSum += (unsigned char)*(lpData+y);
            strcat(buf,buf2);
            }
        rtbDisplay->Lines->Add((AnsiString)buf);
        }
    sprintf(buf2,"Sum = %i",DataSum);
    Label4->Caption = buf2;
    BitBtn19Click(NULL);
    recprg2->Visible = false;
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::MMWomDone(TMessage &Msg)
    {
    recprg2->Visible = false;
     pgr->Progress = pgr->MaxValue;
    Button3->Enabled = true;
    StopButton->Enabled = false;
    RecordButton->Enabled = true;
    TrackBar2->Enabled = true;
    tmrPla->Enabled = false;
    }
//---------------------------------------------------------------------------
/***************************************************************************
Buttons for the recording part
    RecordButton        Starts the recording
    StopButton          Stops the Recording if recording
                        Stops the playback if playing recording
    Button3             Plays the newly created recording
    Button1             Resets the default values for the trackbars
***************************************************************************/
void __fastcall TForm1::RecordButtonClick(TObject *Sender)
    {
    int FreqDivByte;
    //Opens the record at hwi
    if (WaveRecordOpen(&hwi, Handle, 2, 44100, 8))
        {
        //If Open successful sets the header
        if (WaveMakeHeader(ulSize, HData, HWaveHdr, lpData, lpWaveHdr))
            {
            //if header successful begin Recoding
            cntGraph = 0;
     		//grpah->Enabled = true;
            FAllocated = true;
            Rec = true;
            if (WaveRecordBegin(hwi, lpWaveHdr))
                {
                //Set values
                FRecording = true;
                ShowProgress("Recording...");
                //Disable record, play and enable Stop
                StopButton->Enabled = true;
                Button3->Enabled = false;
                RecordButton->Enabled = false;
                //Disables Trackbar2 otherwise the user could case problems
                TrackBar2->Enabled = false;
                //Set progress values
                FreqDivByte = ((ulSize/2)+Freq);
                SubtractAm = ((((int)((float)FreqDivByte*0.05)-(2000))/(FreqDivByte / Freq)) % 100);
                pgr->MaxValue = FreqDivByte/SubtractAm;
                pgr->Progress = pgr->MaxValue;
                //Enables the timer to begin the could down
                tmrRec->Enabled = true;
                }
            }
        }
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::StopButtonClick(TObject *Sender)
    {
    //If recording
    if (FRecording)
        {
        WaveRecordEnd(hwi, lpWaveHdr);
        FRecording = false;
        }
    //If playing
    if (FPlaying)
        {
        WavePlayEnd(hwo, lpWaveHdr);
        FAllocated = true;
        }
    if ((!FPlaying) && (!FRecording))
    	{
        Writing = 1;

        }
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::Button3Click(TObject *Sender)
    {
    int FreqDivByte;
    //Start playing
    if (WavePlayOpen(&hwo, Handle, 2, 44100, 8))
        {
        if (WavePlayBegin(hwo, lpWaveHdr))
            {
            //Set values
            FPlaying = true;
            ShowProgress("Playing...");
            //Calculate progress bar
            pgr->MaxValue = ulSize;
            pgr->Progress = pgr->MaxValue;
            FreqDivByte = ((ulSize/Freq)*20)/10;
            SubtractAm = 20*FreqDivByte;
            pgr->Progress = 0;
            //Enable Stop,Timer Disable Record and Play
            RecordButton->Enabled = false;
            Button3->Enabled = false;
            StopButton->Enabled = true;
            tmrPla->Enabled = true;
            TrackBar2->Enabled = false;
            }
        }
    }

//---------------------------------------------------------------------------

void __fastcall TForm1::Button1Click(TObject *Sender)
    {
    TrackBar2->Position = 10;
    TrackBar1->Position = TrackBar1->Max;
    }
//---------------------------------------------------------------------------
/***************************************************************************
Timers used for the progress bar which shows the how much recording time the
    user has or how close the playback is to the end of the recording.
***************************************************************************/
//---------------------------------------------------------------------------

void __fastcall TForm1::tmrRecTimer(TObject *Sender)
    {//Timer speed is 50ms which is 20Hz
    pgr->Progress -= 5;
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::tmrPlaTimer(TObject *Sender)
    {
    //Timer speed is 50ms which is 20Hz
    pgr->Progress += Freq2;
    }
//---------------------------------------------------------------------------
/***************************************************************************
Track bars used to change how the program records or playsback
Trackbar1 changes the volume
Trackbar2 changes th buffer size allowing the user to record for a longer
    peroid of time.
***************************************************************************/
void __fastcall TForm1::TrackBar2Change(TObject *Sender)
    { //This changes the buffer size of the recording time
    char Buffer[30];
    //The bar has the numbers from 0-100 the defualt buff size is 1000000
    ulSize = (TrackBar2->Position) * 10000;
    //Updates the label
    sprintf(Buffer,"Recording Length\n%i bytes",ulSize);
    lblRecLen->Caption = Buffer;
    }

//---------------------------------------------------------------------------
void __fastcall TForm1::TrackBar1Change(TObject *Sender)
    { //Changes the volume level
    char buffer[30];

    //Changes the volume of the speakers not the computer
    if (!PageRec->Visible)
        {
        DWORD new_volume =(DWORD)MAKEWPARAM(TrackBar1->Position,TrackBar1->Position);
        waveOutSetVolume((HWAVEOUT)WAVE_MAPPER, new_volume);
        sprintf(buffer,"Volume\n%0.2f%",(float)((float)TrackBar1->Position / (float)65535)*100);
        TrackBar3->Position = TrackBar1->Position;
        }
    else
        {
        DWORD new_volume =(DWORD)MAKEWPARAM(TrackBar3->Position,TrackBar3->Position);
        waveOutSetVolume((HWAVEOUT)WAVE_MAPPER, new_volume);
        sprintf(buffer,"Volume\n%0.2f%",(float)((float)TrackBar3->Position / (float)65535)*100);
        TrackBar1->Position = TrackBar3->Position;
        }
    //Updates the labels
    lblVolume->Caption = buffer;
    StaticText1->Caption = buffer;
    }

//---------------------------------------------------------------------------

/***************************************************************************
Basic Display Options
***************************************************************************/
void __fastcall TForm1::btnClearClick(TObject *Sender)
    {
    bool Answer;
    

    }
//---------------------------------------------------------------------------

void __fastcall TForm1::btnFontClick(TObject *Sender)
    {
    bool Answer;
    Answer = fdgDisplay->Execute();
    if (Answer) rtbDisplay->Font = fdgDisplay->Font;
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::btnColourClick(TObject *Sender)
    {
    bool Answer;
     Answer = cdgDisplay->Execute();
     if(Answer) rtbDisplay->Color = cdgDisplay->Color;
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::btnFindClick(TObject *Sender)
    {
    fdg->Execute();
    //Value = FindText(rtbDisplay->Text,0,rtbDisplay->Text.Length,"%%");
    }
//---------------------------------------------------------------------------
/***************************************************************************
Texting Options
***************************************************************************/
void __fastcall TForm1::nudSpecialChange(TObject *Sender)
    {
    char Buffer[30];
    if (nudSpecial->Text != "")
        {
        sprintf(Buffer,"%c",nudSpecial->Value);
        lblSpecial->Caption = Buffer;
        }
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn2Click(TObject *Sender)
    {
    char Buffer[140];
    sprintf(Buffer,"%s%s",rtbTexting->Text,lblSpecial->Caption);
    rtbTexting->Text = Buffer;
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::cbxViewChange(TObject *Sender)
    {
    PrintMessCombo();
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn3Click(TObject *Sender)
    {
    char buff[2];
    long Sig;
    unsigned long nwrit;
    if (Run2 == 1)
        Msmove = NewMess(Mscnt);
    else
        {
        Msmove = Mshead;
        Run2 = 1;
        }
    sprintf(Msmove->mess,"%s",rtbTexting->Text);
    Msmove->length = strlen(Msmove->mess);
    Msmove->senderID = MyID;
    Msmove->Spec = Mscnt-1;
    Msmove->Priority = 7;
    ChangePri(Mscnt-1,nudPrio->Value);
    Msmove->reciverID = nudSendTo->Value;
    if (ConnectState)
    	{
        Sig = 0xDEADBEEF;
        WriteFile(hCom, &Sig,sizeof(long),&nwrit,NULL);

        buff[0] = (char)Msmove->reciverID;
        WriteFile(hCom, buff,1,&nwrit,NULL);

        buff[0] = (char)0x00;
        WriteFile(hCom, buff,1,&nwrit,NULL);

        buff[0] = (char)0x01;
        WriteFile(hCom, buff,1,&nwrit,NULL);

        buff[0] = (char)(Msmove->length + 3);
        WriteFile(hCom, buff,1,&nwrit,NULL);

        WriteFile(hCom,Msmove->mess,strlen(Msmove->mess),&nwrit,NULL);

        buff[0] = (char)Msmove->senderID;
        WriteFile(hCom, buff,1,&nwrit,NULL);
        
        buff[0] = (char)Msmove->Priority;
        WriteFile(hCom,buff,1,&nwrit,NULL);

        buff[0] = (char)0x00;
        WriteFile(hCom, buff,1,&nwrit,NULL);
        }
    PrintMessCombo();
    nudTo->MaxValue = Mscnt-1;
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::rtbTextingChange(TObject *Sender)
    {
    char Buffer[140];
    sprintf(Buffer,"Message Length: %i",rtbTexting->Text.Length());
    lblMessageLength->Caption = Buffer;
    if ((rtbTexting->Text.Length() >= 140) && (Run1 == 0))
        {
        sprintf(Buffer,"Your message has reach its limit of 140 characters!");
        MessageBox(NULL,Buffer,"",MB_OK);
        Run1 = 1;
        }
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn4Click(TObject *Sender)
    {
    BitBtn4->Enabled = false;
    closecomm();
    sprintf(COMname,"COM%i",COMNum->Value);
    opencomm();
    BitBtn4->Enabled = true;
    }
//---------------------------------------------------------------------------
void __fastcall TForm1::BitBtn8Click(TObject *Sender)
    {
    BitBtn4->Enabled = false;
    closecomm();
    sprintf(COMname,"COM%i",COMNum->Value);
    COMspeedV = COMspeed->Value;
    opencomm();
    BitBtn4->Enabled = true;
    }
//-------------------------------------------------------------------------
void __fastcall TForm1::fdgFind(TObject *Sender)
    {
    rtbDisplay->Font->Color = clNavy;
    sprintf(OldFindText,"%s",fdg->FindTextA);
    FindText(OldFindText,clBlue);
    }


//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn10Click(TObject *Sender)
    {
    if (PageSen->Visible)
        {
        PageSen->Visible = false;
        PageRec->Visible = true;
        }
    else
        {
        PageSen->Visible = true;
        PageRec->Visible = false;
        }
    }
//---------------------------------------------------------------------------
void __fastcall TForm1::nudFromChange(TObject *Sender)
    {
    PrintMessCombo();
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::nudToChange(TObject *Sender)
    {
    PrintMessCombo();
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::MyIDEditChange(TObject *Sender)
    {
    MyID = MyIDEdit->Value;
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn5Click(TObject *Sender)
    {
    char Buffer[100];
    int x;
    if (ConnectState == false)
    	{
    	BitBtn4->Enabled = false;
	    BitBtn5->Enabled = false;
    	ConnectState = false;
    	for (x=0;(ConnectState!=true) && (x<10);x++)
        	{
        	closecomm();
        	sprintf(COMname,"COM%i",x);
        	opencomm();
        	}
    	if ((x==10) && (ConnectState!=true))
        	{
        	sprintf(Buffer,"Unable to find a serial port!");
        	MessageBox(NULL,Buffer,"",MB_OK);
        	}
    	else
        	{
        	CSpinEdit1->Value = x-1;
            COMNum->Value = x-1;
        	}
    	BitBtn4->Enabled = true;
    	BitBtn5->Enabled = true;
        }
    else
       {
       sprintf(Buffer,"Already connected");
       MessageBox(NULL,Buffer,"",MB_OK);
       }
	}
//---------------------------------------------------------------------------

/*void __fastcall TForm1::rtbDisplayMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
	{
    int loca;
    rtbDisplay->SetFocus();
    loca = (X) + (rtbDisplay->Font->Height - Y);
    //if (loca < 0)
      	//loca *= -1;
    //rtbDisplay->SelStart = loca;
	} */
//---------------------------------------------------------------------------

void __fastcall TForm1::RecMesBoxClick(TObject *Sender)
	{
    messagetp *fnd;
    fnd = FinMess(RecMesBox->ItemIndex);
    mmoDisplayText->Text = fnd->mess;
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn11Click(TObject *Sender)
	{
    messagetp *fnd;
    fnd = FinMess(RecMesBox->ItemIndex);
    mmoDisplayText->Text = fnd->mess;
    cbxView->ItemIndex = 8;
    nudFrom->Value = RecMesBox->ItemIndex;
    nudTo->Value = RecMesBox->ItemIndex;
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::btnDeleteMClick(TObject *Sender)
	{
    messagetp *Moving;
    char buff[100];
    int idx = 0;
    DelMess(RecMesBox->ItemIndex);
    RecMesBox->Items->Clear();
    for (Moving = Mshead;Moving!=Mstail->next;Moving=Moving->next)
    	{
        sprintf(buff,"Message #%i: From %i, To %i, Priority %i",idx,Moving->senderID,Moving->reciverID,Moving->Priority);
    	Form1->RecMesBox->Items->Add(buff);
        idx++;
        }
    Numdel++;
    sprintf(buff,"%i Messages have been deleted",Numdel);
    Form1->lblTotalDeleted->Caption = buff;
    PrintMessCombo();
	}
//---------------------------------------------------------------------------


void __fastcall TForm1::CSpinEdit1Change(TObject *Sender)
	{
    COMNum->Value = CSpinEdit1->Value;
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::CSpinEdit2Change(TObject *Sender)
	{
    if (CSpinEdit2->SelLength != NULL)
    	COMspeed->Value = CSpinEdit2->Value;
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button4Click(TObject *Sender)
    {
    rxthr = new RxThread(true);
    rxthr->Priority = tpHighest;
    rxthr->Resume(); // Start - up rx thread
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn6Click(TObject *Sender)
    {
    int x;
    free(Mshead);
    Mscnt = 0;
    for(x=0;x<7;x++)
        Picnt[x] = 0;
    rtbDisplay->Text = " ";
    }
//---------------------------------------------------------------------------
void __fastcall RxThread::ViewPrg2()
    {
    Form1->recprg2->Left = (Form1->Width / 2) - (Form1->recprg2->Width / 2);
    Form1->recprg2->Top = (Form1->Height / 2) - (Form1->recprg2->Height / 2);
    Form1->recprg2->Visible = true;
    }

void __fastcall TForm1::BitBtn12Click(TObject *Sender)
	{
    char buff[2];
    char buff2[100];
    unsigned long nwrit;
    int x;
    if (Run2 == 1)
        Msmove = NewMess(Mscnt);
    else
        {
        Msmove = Mshead;
        Run2 = 1;
        }
    sprintf(Msmove->mess,"%s",rtbTexting->Text);
    Msmove->length = strlen(Msmove->mess);
    Msmove->senderID = MyID;
    Msmove->Spec = Mscnt-1;
    Msmove->Priority = 7;
    ChangePri(Mscnt-1,nudPrio->Value);
    Msmove->reciverID = nudSendTo->Value;
    if (ConnectState)
    	{
        Head.lSignature = 0xDEADBEEF;
        Head.bReceiverAddr = Msmove->reciverID;
        Head.bVersion = 1;
         if (CompressedM == 0)
        	Head.lDataLength = strlen(Msmove->mess)+7;
        else
            Head.lDataLength = Hold[1]+7;
        Head.bPattern[0] = 0xAA,  Head.bPattern[1] = 0x55,  Head.bPattern[2] = 0xAA,  Head.bPattern[3] = 0x55;
        WriteFile(hCom, &Head,sizeof(struct Header),&nwrit,NULL);

        buff[0] = 1;
        WriteFile(hCom, buff,1,&nwrit,NULL);
        
        if (CheckBox12->Checked)
             CompressedM |= 0x04;
        else
        	CompressedM &= 0xFB;

         DataSum = 0;

        if ((CompressedM == 0) || (CompressedM == 4))
           	for(x=0;x<=Head.lDataLength-7;x++)
            	Compress3[x] = Msmove->mess[x];
        else
            for(x=0;x<=Head.lDataLength-7;x++)
            	Compress3[x] = Compress2[x];

        if (CheckBox12->Checked)
        	for(x=0;x<=Head.lDataLength-7;x++)
            	Compress3[x] = Compress3[x] ^ 0x32;

        for(x=0;x<Head.lDataLength-7;x++)
            DataSum += Compress3[x];

        sprintf(buff2,"Sum = %i",DataSum);
        Label14->Caption = buff2;

        WriteFile(hCom,Compress3,Head.lDataLength-7,&nwrit,NULL);

        WriteFile(hCom,&DataSum,sizeof(long),&nwrit,NULL);

        buff[0] = (char)Msmove->senderID;
        WriteFile(hCom, buff,1,&nwrit,NULL);

        buff[0] = (char)Msmove->Priority;
        WriteFile(hCom,buff,1,&nwrit,NULL);

        buff[0] = CompressedM;
        WriteFile(hCom,buff,1,&nwrit,NULL);
        if ((CompressedM & 0x04) != 0)//Send encryption key
        	{
            buff[0] = 0x32;
            WriteFile(hCom,buff,1,&nwrit,NULL);
        	}
        if ((CompressedM & 0x02) != 0) //Send Huffman Tree
            {
            WriteFile(hCom,&cntTrees,1,&nwrit,NULL); //Number of trees
            WriteFile(hCom,&Msmove->length,1,&nwrit,NULL);
            SendTree(root);
            }
        if ((CompressedM & 0x01) != 0) //Send Escape key
           {
           buff[0] = (char)Hold[0];
           WriteFile(hCom,buff,1,&nwrit,NULL);
           }
        }
    PrintMessCombo();
    nudTo->MaxValue = Mscnt-1;
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn13Click(TObject *Sender)
	{
    closecomm();
    ckbcomopen->Checked = false;
    ckbCOMState->Checked = false;
    ckbCOMSet->Checked = false;
    ckbCOMTime->Checked = false;
    ckbCOMTimeWrite->Checked = false;
    Shape2->Brush->Color = clWhite;

    CheckBox3->Checked = false;
    CheckBox4->Checked = false;
    CheckBox5->Checked = false;
    CheckBox6->Checked = false;
    CheckBox7->Checked = false;
    Shape1->Brush->Color = clWhite;
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn15Click(TObject *Sender)
	{
    int x;
    for(x=0;x<1000;x++)
    	RawData[x] = 0;
    RawIdx=0;
    PrintMessCombo();
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn16Click(TObject *Sender)
	{
    RawData[RawIdx] = '\n';
    RawIdx++;
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button6Click(TObject *Sender)
	{
    int x;
    rtbDisplay->Visible = false;
    Splitter1->Visible = false;
    RecordBox->Visible = false;
    Writing = 0;
   
    Writing = 1;
    //PerformanceGraph1->BeginDrag(true,30);

	}
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn18Click(TObject *Sender)
	{
    if (rtbDisplay->Visible)
    	{
    	rtbDisplay->Visible = false;
    	Splitter1->Visible = false;
    	RecordBox->Visible = false;
    	}
    else
    	{
        RecordBox->Visible = true;
        Splitter1->Visible = true;
        rtbDisplay->Visible = true;
        }
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::grpahTimer(TObject *Sender)
	{
    recprg2->Visible = false;
    grpah->Enabled = false;
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn19Click(TObject *Sender)
	{
    int i;
    if (Writing != 0) return;
    TChartSeries* series1 = Chart1->Series[0];
   	series1->Clear();
    Chart1->BottomAxis->Maximum = ulSize;
    for(i=0;i<ulSize;i++)
        series1->AddXY(i, (unsigned char)*(lpData+i), "", clGreen);
    Chart1->Update();
    StopButton->Enabled = true;
    //grpah->Enabled = true;
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn17Click(TObject *Sender)
	{
    if (freeze == 0)
   		freeze = 1;
    else
    	freeze = 0;

    PrintMessCombo();
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::ComboBox2Change(TObject *Sender)
	{
	PrintMessCombo();
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::CSpinEdit3Change(TObject *Sender)
	{
    PrintMessCombo();
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::CSpinEdit4Change(TObject *Sender)
	{
    int x;
    char buf[20];
    Max[1] = 0x80;
    Min[1] = 0x80;

	}
//---------------------------------------------------------------------------



void __fastcall TForm1::CSpinEdit6Change(TObject *Sender)
    {
   // PerformanceGraph1->StepSize = CSpinEdit6->Value;
    }
//---------------------------------------------------------------------------


void __fastcall TForm1::BitBtn20Click(TObject *Sender)
	{
    char buf[75];
    int x;
    ShowProgress("Compression");
    if (CheckBox10->Checked)
       {
       UpdateProgress("Cleaning Silence");
       for(x=0;x<ulSize;x++)
           {
           if((((unsigned char)*(lpData+x)) > 0x80) && (((unsigned char)*(lpData+x)) <= 0x83))
               *(lpData+x) = 0x80;
           if((((unsigned char)*(lpData+x)) >= 0x7D) && (((unsigned char)*(lpData+x)) < 0x80))
               *(lpData+x) = 0x80;
           }
       }
	if (CheckBox9->Checked)
    	{
        UpdateProgress("RLE Compression");
        CompressedR |= 0x0001;
        if (!CheckBox8->Checked)
            Compress(lpData,ulSize,Compress2,Hold);
        else
            Compress(lpData,ulSize,Compress3,Hold);
        sprintf(buf,"From %i to %i, Escape: %02X",ulSize,Hold[1],Hold[0]);
        Label1->Caption = buf;
        }
    else
       CompressedR &= 0xFE;
    if (CheckBox8->Checked)
        {
        CompressedR |= 0x0002;
        UpdateProgress("Huffman Compresstion");
        if (!CheckBox9->Checked)
            ComHuffman(lpData,ulSize,Compress2,&Hold[1]);
        else
            ComHuffman(Compress3,Hold[1],Compress2,&Hold[1]);
        sprintf(buf,"From %i to %i",ulSize,Hold[1],Hold[0]);
        Label1->Caption = buf;
        }
    else
       CompressedR &= 0xFD;
    UpdateProgress("Updating States");
    DataSum = 0;
    for(x=0;x<Hold[1];x++)
       {
       DataSum += Compress2[x];
       }
    sprintf(buf,"Sum = %i",DataSum);
    Label4->Caption = buf;
    recprg2->Visible = false;
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn22Click(TObject *Sender)
    {
    int x,y;
    char buf[200];
    char buf2[32];
    rtbDisplay->Text = "";
    for(x=0;x<Hold[1];x+=32)
    	{
       	strcpy(buf,"");
        for(y=x;y<(x+32);y++)
        	{
            if (y < Hold[1])
                {
                sprintf(buf2,"%02X ",Compress2[y]);
                strcat(buf,buf2);
                }
            }
        rtbDisplay->Lines->Add((AnsiString)buf);
        }
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn21Click(TObject *Sender)
    {
    char buf[75];
    unsigned char buff[140];
    int x;
    sprintf(buff,"%s",rtbTexting->Text);
	if (CheckBox1->Checked)
    	{
        CompressedM |= 0x0001;
        if (!CheckBox2->Checked)
            Compress(buff,rtbTexting->Text.Length(),Compress2,Hold);
        else
            Compress(buff,rtbTexting->Text.Length(),Compress3,Hold);
        sprintf(buf,"From %i to %i, Escape: %02X",rtbTexting->Text.Length(),Hold[1],Hold[0]);
        Label3->Caption = buf;
        }
    else
       CompressedM &= 0xFE;
    if (CheckBox2->Checked)
        {
        CompressedM |= 0x0002;
        if (!CheckBox1->Checked)
            ComHuffman(buff,rtbTexting->Text.Length(),Compress2,&Hold[1]);
        else
            ComHuffman(Compress3,Hold[1],Compress2,&Hold[1]);
        sprintf(buf,"From %i to %i",rtbTexting->Text.Length(),Hold[1],Hold[0]);
        Label3->Caption = buf;
        }
    else
       CompressedM &= 0xFD;
    }
//---------------------------------------------------------------------------

void __fastcall TForm1::BitBtn1Click(TObject *Sender)
	{
    char buff[2];
    int x;
    unsigned long nwrit;
    if (ConnectState)
    	{
        ShowProgress("Sending Recording");
        recprg2->Repaint();
        Head.lSignature = 0xDEADBEEF;
        Head.bReceiverAddr = RecIDr->Value;
        Head.bVersion = 1;
        if (CompressedR == 0)
        	Head.lDataLength = ulSize+7;
        else
            Head.lDataLength = Hold[1]+7;
        Head.bPattern[0] = 0xAA,  Head.bPattern[1] = 0x55,  Head.bPattern[2] = 0xAA,  Head.bPattern[3] = 0x55;
        WriteFile(hCom, &Head,sizeof(struct Header),&nwrit,NULL);

        buff[0] = 0;
        WriteFile(hCom, buff,1,&nwrit,NULL);

        DataSum = 0;

        if (CheckBox11->Checked)
           CompressedR |= 0x04;
        else
           CompressedR &= 0xFB;

       	if ((CompressedR == 0) || (CompressedR == 4))
            for(x=0;x<=ulSize;x++)
            		Compress3[x] = ((unsigned char)*(lpData+x));
        else
            for(x=0;x<=Hold[1];x++)
            		Compress3[x] = Compress2[x];

        if (CheckBox11->Checked)
        	for(x=0;x<=ulSize;x++)
            	Compress3[x] = Compress3[x] ^ 0x32;

        for(x=0;x<Head.lDataLength-7;x++)
            	DataSum += Compress3[x];

        if ((CompressedR == 0) || (CompressedR == 4))
        	WriteFile(hCom,lpData,ulSize,&nwrit,NULL);
        else
            WriteFile(hCom,Compress2,Hold[1],&nwrit,NULL);

        WriteFile(hCom,&DataSum,sizeof(long),&nwrit,NULL);

        buff[0] = (char)MyIDEdit->Value;
        WriteFile(hCom, buff,1,&nwrit,NULL);

        buff[0] = (char)0;
        WriteFile(hCom,buff,1,&nwrit,NULL);

        buff[0] = CompressedR;
        WriteFile(hCom,buff,1,&nwrit,NULL);
        if ((CompressedR & 0x04) != 0)//Send encryption key
        	{
            buff[0] = 0x32;
            WriteFile(hCom,buff,1,&nwrit,NULL);
        	}
        if ((CompressedR & 0x02) != 0) //Send Huffman Tree
            {
            WriteFile(hCom,&cntTrees,2,&nwrit,NULL); //Number of trees
            WriteFile(hCom,&ulSize,sizeof(long),&nwrit,NULL); //Number of trees
            SendTree(root);
            }
        if ((CompressedR & 0x01) != 0) //Send Escape key
           {
           buff[0] = (char)Hold[0];
           WriteFile(hCom,buff,1,&nwrit,NULL);
           }

        }
    recprg2->Visible = false;
    PrintMessCombo();
    nudTo->MaxValue = Mscnt-1;
	}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button5Click(TObject *Sender)
	{
	int x;
    rectp *New;
    if (WaveRecordOpen(&hwi, Handle, 2, 44100, 8))
        {
        //If Open successful sets the header
        if (WaveMakeHeader(ulSize, HData, HWaveHdr, lpData, lpWaveHdr))
       		{
            New = (rectp *)malloc(sizeof(rectp));
            New->Size = NumChar;
            New->Data = (char *)malloc(NumChar);
            New->reciverID = Msen;
    		for(x=0;x<NumChar;x++)
    			*(New->Data + x) = rbuf[x];
            RcAdd(New);
            BitBtn19Click(NULL);
            }
    	}
	}
//---------------------------------------------------------------------------



void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
    {
    int x=0;
    Fil = fopen(FILENAME, "wb");
    if (!Fil) return;
    fwrite(&Mscnt,sizeof(int),1,Fil);
    if (!Mshead) return;
    for (Msmove = Mshead;Msmove != Mstail->next;Msmove = Msmove->next,x++)
        {
        fwrite(&x,sizeof(int),1,Fil);
        fwrite(&Msmove->senderID,sizeof(int),1,Fil);
        fwrite(&Msmove->reciverID,sizeof(int),1,Fil);
        fwrite(&Msmove->Priority,sizeof(int),1,Fil);
        fwrite(&Msmove->Spec,sizeof(int),1,Fil);
        fwrite(&Msmove->length,sizeof(int),1,Fil);
        fwrite(&Msmove->mess,Msmove->length,1,Fil);
        }
    fclose(Fil);
    }
//---------------------------------------------------------------------------


void __fastcall TForm1::CheckListBox1Click(TObject *Sender)
	{
    rectp *Clicked;
   	int x = 0;
    for(Clicked = RcHead;Clicked!=RcTail->Next, x != Form1->CheckListBox1->ItemIndex;Clicked=Clicked->Next)
       x++;
    for(x=0;x<Clicked->Size;x++)
    	*(lpData + x) = *(Clicked->Data + x);
    Writing = 0;
    BitBtn19Click(NULL);
	}
//---------------------------------------------------------------------------

