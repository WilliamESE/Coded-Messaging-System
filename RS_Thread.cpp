#include <vcl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma hdrstop

#include "RS_Thread.h"
#include "Unit1.h"
#pragma package(smart_init)

extern HANDLE hCom;
extern unsigned char rbuf[100000];
extern char RawData[1000];
extern int RawIdx;
extern unsigned char Compress2[100000];
extern unsigned char Compress3[100000];
extern int Mrec, Msen, Mpri, Mtype, Mtyp;
extern unsigned long Mlen ,NumChar;
extern int Hold[2];
extern int SaveHuff[256][2];
int haverbuf;

typedef struct HuffTree
	{
	char type;
	int Value;
	unsigned char chara;
	struct HuffTree *Left;
	struct HuffTree *Right;
	}hufftp;

extern hufftp *root;

void CreateTree(char Chara, char NumBits,unsigned short Bits)
	{
    int x, cntB;
    char bit;
    hufftp *moveC;
    if (root == NULL)
    	{
    	root = (hufftp *)malloc(sizeof(hufftp));
        root->Right = NULL;
        root->Left = NULL;
        root->type = 1;
    	}
    if (!root) return;
    moveC = root;
    cntB = 0;
    if (NumBits <= 3)
    	cntB = 0;
    for (x=(pow(2,15-NumBits));(x<=0x8000) && (cntB <= NumBits);x*=2,cntB++)
    	{
        bit = 1;
        if (!(Bits & x)) bit = 0;
        if (bit)
        	{
            if (!moveC->Right)
            	{
				moveC->Right = (hufftp *)malloc(sizeof(hufftp));
                moveC->Right->Right = NULL;
                moveC->Right->Left = NULL;
            	}
            moveC->Right->type = 1;
            moveC = moveC->Right;
        	}
        if (!bit)
        	{
            if (!moveC->Left)
            	{
            	moveC->Left = (hufftp *)malloc(sizeof(hufftp));
                moveC->Left->Right = NULL;
                moveC->Left->Left = NULL;
            	}
            moveC->Left->type = 1;
            moveC = moveC->Left;
        	}
        }
    moveC->chara = Chara;
    moveC->type = 0;
    }

//Decompresses the RLE compression
int Decompress(unsigned char In[], int Size,unsigned char Output[], unsigned char Escape)
	{
	int x;
	int y;
	int outx=0;
	int cnt;
	int state=0;
	for(x=1;x<Size;x++)
		{
		//Check for Escape
		if (In[x] == Escape)
			state = 1;
		switch (state)
			{
		    case 0: //Add to the output normally
				Output[outx] = In[x];
				outx++;
				break;
			case 1: //Get the number of the bits after escape
				x++;
				cnt = (int)In[x];
				state = 2;
				break;
			case 2: //Add the number of bits for escape
				for(y=0;y<(cnt);y++,outx++)
					Output[outx] = In[x];
				state = 0;
				break;
			}
		}
    return outx;
	}

void ShowProgress2(char Test[])
    {
    Form1->Panel2->Top = (Form1->Height / 2) - (Form1->Panel2->Height / 2);
    Form1->Panel2->Left = (Form1->Width / 2) - (Form1->Panel2->Width / 2);
    Form1->Label13->Caption = Test;
    Form1->Panel2->Visible = true;
    }

void UpdateProgress2(char Test[])
	{
	Form1->Label13->Caption = Test;
	Form1->Panel2->Update();
	}

__fastcall RxThread::RxThread(bool CreateSuspended)
    : TThread(CreateSuspended)
	{
	//RxThread *Event1 = new TEvent(NULL,true,false,"Test_Event");
	//*TEvent new *TEvent Event1(NULL,true,false,"Test_Event");
	}

void __fastcall RxThread::Execute()
	{
    DWORD nrd;
    int x;
    int a;
    int State = 0;
	unsigned long Longer=0;
	long Len;
    long Rlen;
    int Mtype = 0;
    int Extra1;
    char Enpt = 0;
    char RLEscape = 0;
    int NumTree;
    int Nididx;
    unsigned long DataSumR;
    unsigned long DataSumS;
	int cnt;
	char Rcom;
    unsigned char buff[256];
    COMMTIMEOUTS CommTim;
    Mlen = 0;

    CommTim.ReadIntervalTimeout = MAXWORD;
    CommTim.ReadTotalTimeoutMultiplier = MAXWORD;
    CommTim.WriteTotalTimeoutConstant = 125;
    SetCommTimeouts(hCom,&CommTim);

    while(!Terminated)
    	{
        ReadFile(hCom,buff,1,&nrd,NULL);
        if (nrd != 0)
        	{
            if (RawIdx == 1000)
            	RawIdx = 0;
            RawData[RawIdx] = *buff;
            RawIdx++;
            switch(State)
				{
				case 0: //Get header
                    rbuf[cnt] = *buff;
                    Longer += (unsigned char)*buff;
                    cnt++;
					if (Longer == 824)
						{
						State=1;
						}
					if (cnt > 3)
                        {
						cnt = 0;
                        Longer = 0;
                        }
					break;
				case 1: //Get receiver ID
                    rbuf[cnt] = *buff;
                    cnt++;
                    Longer = 0;
					if (cnt >= 1)
						{
						Mrec = buff[0];
						if ((Mrec == 0) || (Mrec == Form1->CSpinEdit5->Value))
							{
							State = 2;
							ShowProgress2("Receiving Data");
                            Form1->Panel2->Update();
							}
						else
							State = 0;
						}
					cnt = 0;
					break;
				case 2: //Version
                    rbuf[cnt] = *buff;
                    cnt++;
                    Mlen = 0;
					if (cnt >= 1)
						if (buff[0] == 1)
							State = 3;
						else
                            {
							State = 0;
                            UpdateProgress2("Version Failed");
                            }
					cnt = 0;
					break;
				case 3: //Data Length
               		if (cnt == 0)
                    	Mlen = buff[0];
                    else
				   		Mlen = Mlen + (buff[0] * (256 * pow(256,(cnt-1))));

					cnt++;
					if (cnt >= sizeof(long))
						{
						State=4;
						cnt = 0;
						}
					break;
				case 4: //Header End
                    rbuf[cnt] = *buff;
                    Longer += (unsigned char)*buff;
                    cnt++;
					if (Longer == 510)
             			{
						State = 5;
                        cnt = 0;
                    	}
					if ((cnt > 3) && (State != 8))
                        {
						cnt = 0;
                        UpdateProgress2("Header Ending Failed");
                        State = 0;
                        }
					break;
				case 5: //Type
                    Mtyp = buff[0];
                    cnt++;
					if ((Mtyp == 1) || (Mtyp == 0))
						{
						Mtyp = buff[0];
						State = 6;
                        DataSumR = 0;
                        Longer = 0;
						}
					else
						State = 0;
					cnt = 0;
					break;
				case 6: //Message
                    rbuf[cnt] = *buff;
                    DataSumR += rbuf[cnt];
                    cnt++;
					if (cnt >= (Mlen-7))
						{
						cnt=0;
                        DataSumS = 0;
						State=7;
						}
					break;
                case 7: //Sum
               		if (cnt == 0)
                    	DataSumS = buff[0];
                    else
				   		DataSumS = DataSumS + (buff[0] * (256 * pow(256,(cnt-1))));

					cnt++;
					if (cnt >= sizeof(long))
						{
                       // if (DataSumS == DataSumR)
                         //	{
							State=8;
							cnt = 0;
                           //	}
                        //else
                          // {
                          // UpdateProgress2("Sums don't match!");
                          // State = 0;
                          // }
						}
					break;
				case 8: //Sender
                    cnt++;
					if (cnt >= 1)
						{
						Msen = buff[0];
						State = 9;
						}
					cnt = 0;
					break;
				case 9: //Priority
                    cnt++;
					if (cnt >= 1)
						{
						Mpri = buff[0];
						State = 10;
						}
					cnt = 0;
					break;
				case 10: //Extra
                    cnt++;
                    Extra1 = 0;
                    Extra1 = buff[0];
                    NumChar = Mlen-7;
                    NumTree = 0;
					if (cnt >= 1)
						{
						if(buff[0] == 0)
							{
                            UpdateProgress2("Writing Data");
							Synchronize(AddItReciver);
                            Form1->Panel2->Visible = false;
                            State = 0;
							}
						else
							{
                            if((Extra1 & 0x04) != 0) //The data is encrypted
                                State = 11;
                            else if((Extra1 & 0x02) != 0)
                                State = 13;
                            else if((Extra1 & 0x01) != 0)
                                State = 12;
							}
                        Longer = 0;
						}
					cnt = 0;
					break;
                case 11: //encryption key
                    Enpt = *buff;
                    for(x=0;x<(Mlen-7);x++)
                        rbuf[x] = rbuf[x] ^ Enpt;
                    if((Extra1 & 0x02) != 0)
                    	State = 13;
                    else if((Extra1 & 0x01) != 0)
                    	State = 12;
                    else
                       {
                       UpdateProgress2("Writing Data");
					   Synchronize(AddItReciver);
                       Form1->Panel2->Visible = false;
                       State = 0;
                       }
                    break;
                case 12: //RLE Escape
					UpdateProgress2("Decompress RLE");
                    if(!(Extra1 & 0x02))
                    	Hold[1] = Decompress(rbuf,Mlen-7,Compress2, (unsigned char)buff[0]);
                    else
                    	{
                        Hold[1] = Decompress(Compress3,Hold[1],Compress2, (unsigned char)buff[0]);
                    	}
					UpdateProgress2("Writing Data");
                    for(x=0;x<=Hold[1];x++)
                         rbuf[x] = Compress2[x];
                    NumChar = Hold[1];
                    Synchronize(AddItReciver);
                    Form1->Panel2->Visible = false;
                    State = 0;
                    break;
                case 13: //Number of nodes
                	UpdateProgress2("Collecting Tree");
                    NumTree = *buff;
                    x = 0;
                    State = 14;
                    Hold[0] = 0;
                    cnt = 0;
                    break;
                case 14: //Number of characters
                    NumChar = buff[0];
                   	State = 15;
                    cnt=0;
                    if (!Mtyp)
                         NumChar *= 10000;
                    break;
                case 15: //Node Index
                    Nididx = *buff;
                    State = 16;
                    break;
                case 16: //Number of bits
                    SaveHuff[Nididx][0] = *buff;
                    State = 17;
                    break;
                case 17: //bits

                    if (cnt == 0)
                    	SaveHuff[Nididx][1] = *buff;
                    else
				   		SaveHuff[Nididx][1] = SaveHuff[Nididx][1] + (buff[0] * (256 * pow(256,(cnt-1))));
                    cnt++;
                    if (cnt == 2)
                    	{
                    	CreateTree(Nididx, SaveHuff[Nididx][0],SaveHuff[Nididx][1]);
                        State = 15;
                        cnt = 0;
                        x++;
                    	}
                    if (x >= (NumTree))
                    	{
                        UpdateProgress2("Decompressing Huffman");
                        Synchronize(DeComHuffman);
                    	if((Extra1 & 0x01) != 0)
                    		State = 12;
                        else
                        	{
                            UpdateProgress2("Writing Data");
                            for(x=0;x<NumChar;x++)
                            	rbuf[x] = Compress3[x];
                    		Synchronize(AddItReciver);
                    		Form1->Panel2->Visible = false;
                            State = 0;
                            }
                    	}

                    break;
				}
            }
        }
	}



