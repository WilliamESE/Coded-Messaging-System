//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "CGAUGES.h"
#include <ComCtrls.hpp>

#include <mmsystem.h>
#include <ExtCtrls.hpp>
#include <Buttons.hpp>
#include <Menus.hpp>
#include <CheckLst.hpp>
#include "CSPIN.h"
#include <Dialogs.hpp>
#include <DBLookup.hpp>
#include <FileCtrl.hpp>
#include <ExtDlgs.hpp>
#include "PERFGRAP.h"
#include "CGRID.h"
#include <Chart.hpp>
#include <Series.hpp>
#include <TeEngine.hpp>
#include <TeeProcs.hpp>
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
    TTimer *tmrRec;
    TTimer *tmrPla;
    TPageControl *PageSen;
        TTabSheet *TabSheet1;
        TTabSheet *TabSheet2;
        TCGauge *pgr;
        TTrackBar *TrackBar1;
        TTrackBar *TrackBar2;
        TButton *Button1;
        TBitBtn *Button3;
        TBitBtn *RecordButton;
        TBitBtn *StopButton;
        TStaticText *lblVolume;
        TStaticText *lblRecLen;
        TPanel *Panel1;
        TGroupBox *RecordBox;
        TSplitter *Splitter1;
        TGroupBox *SendRec;
        TCSpinEdit *RecIDr;
        TLabel *Label2;
        TBitBtn *BitBtn1;
        TGroupBox *GroupBox1;
        TBitBtn *btnPlay;
        TBitBtn *btnStop;
    TGroupBox *gpbMessage;
        TTabSheet *TabSheet3;
        TBitBtn *btnClear;
        TBitBtn *btnFont;
        TBitBtn *btnColour;
        TBitBtn *btnFind;
        TLabel *Label5;
        TComboBox *cbxView;
        TBitBtn *btnHelp;
        TPanel *pnlViewFrom;
        TLabel *Label6;
        TCSpinEdit *nudFrom;
        TLabel *Label7;
        TCSpinEdit *nudTo;
        TGroupBox *GroupBox3;
        TLabel *lblMessageLength;
    TFontDialog *fdgDisplay;
    TColorDialog *cdgDisplay;
    TGroupBox *GroupBox4;
    TCSpinEdit *nudSpecial;
    TLabel *lblSpecial;
    TLabel *Label9;
    TBitBtn *BitBtn2;
    TBitBtn *BitBtn3;
    TMemo *rtbTexting;
    TGroupBox *GroupBox2;
    TCSpinEdit *nudSendTo;
    TGroupBox *Priority;
    TCSpinEdit *nudPrio;
    TLabel *Label8;
    TLabel *Label10;
    TGroupBox *GroupBox5;
    TCSpinEdit *MyIDEdit;
	TGroupBox *ComPortOption;
    TCSpinEdit *COMNum;
    TBitBtn *BitBtn4;
    TGroupBox *GroupBox7;
    TCheckBox *ckbcomopen;
    TCheckBox *ckbCOMState;
    TCheckBox *ckbCOMSet;
    TCheckBox *ckbCOMTime;
    TCheckBox *ckbCOMTimeWrite;
    TGroupBox *Clearing;
    TBitBtn *BitBtn6;
    TBitBtn *BitBtn7;
    TLabel *Messages;
    TLabel *Recordings;
    TLabel *Speed;
    TCSpinEdit *COMspeed;
    TPageControl *PageRec;
    TTabSheet *TabSheet4;
    TTabSheet *TabSheet5;
    TTabSheet *TabSheet6;
	TListBox *RecMesBox;
    TBitBtn *btnDeleteM;
    TGroupBox *GroupBox8;
    TLabel *lblMessTotal;
    TLabel *lblTotalError;
    TLabel *lblTotalDeleted;
    TCGauge *CGauge1;
    TBitBtn *BitBtn10;
    TStaticText *StaticText1;
    TTrackBar *TrackBar3;
    TFindDialog *fdg;
    TGroupBox *States;
    TLabel *lblRecTotal;
    TLabel *lblRecPlayed;
    TBitBtn *BitBtn5;
	TMemo *mmoDisplayText;
	TBitBtn *BitBtn11;
	TGroupBox *GroupBox6;
	TLabel *Label11;
	TCSpinEdit *CSpinEdit1;
	TCSpinEdit *CSpinEdit2;
	TBitBtn *BitBtn8;
	TBitBtn *BitBtn9;
	TGroupBox *GroupBox9;
	TCheckBox *CheckBox3;
	TCheckBox *CheckBox4;
	TCheckBox *CheckBox5;
	TCheckBox *CheckBox6;
	TCheckBox *CheckBox7;
    TShape *Shape1;
    TShape *Shape2;
    TPanel *recprg2;
    TLabel *Label12;
    TRichEdit *RichEdit1;
    TOpenPictureDialog *opg;
	TBitBtn *BitBtn12;
	TBitBtn *BitBtn13;
	TBitBtn *BitBtn14;
	TTabSheet *TabSheet7;
	TGroupBox *GroupBox10;
	TBitBtn *BitBtn15;
	TBitBtn *BitBtn17;
	TBitBtn *BitBtn16;
	TGroupBox *GroupBox11;
	TComboBox *ComboBox2;
	TRichEdit *rtbDisplay;
	TBitBtn *BitBtn18;
	TTimer *grpah;
	TBitBtn *BitBtn19;
	TCSpinEdit *CSpinEdit3;
	TTabSheet *TabSheet8;
    TCheckBox *CheckBox8;
    TCheckBox *CheckBox9;
    TBitBtn *BitBtn20;
    TGroupBox *GroupBox16;
    TCheckBox *CheckBox1;
    TCheckBox *CheckBox2;
    TBitBtn *BitBtn21;
    TLabel *Label3;
    TLabel *Label1;
    TBitBtn *BitBtn22;
    TBitBtn *BitBtn23;
	TChart *Chart1;
	TLineSeries *Series1;
    TLabel *Label4;
    TCheckBox *CheckBox10;
	TButton *Button5;
	TCSpinEdit *CSpinEdit5;
	TPanel *Panel2;
	TLabel *Label13;
	TBitBtn *BitBtn24;
	TCheckBox *CheckBox11;
	TCheckBox *CheckBox12;
	TLabel *Label14;
    TBitBtn *BitBtn25;
    TLabel *Label15;
	TListBox *CheckListBox1;
        void __fastcall RecordButtonClick(TObject *Sender);
        void __fastcall StopButtonClick(TObject *Sender);
        void __fastcall Button3Click(TObject *Sender);
        void __fastcall TrackBar1Change(TObject *Sender);
    void __fastcall tmrRecTimer(TObject *Sender);
    void __fastcall tmrPlaTimer(TObject *Sender);
    void __fastcall TrackBar2Change(TObject *Sender);
    void __fastcall Button1Click(TObject *Sender);
        void __fastcall btnClearClick(TObject *Sender);
    void __fastcall btnFontClick(TObject *Sender);
    void __fastcall btnColourClick(TObject *Sender);
    void __fastcall btnFindClick(TObject *Sender);
    void __fastcall nudSpecialChange(TObject *Sender);
    void __fastcall BitBtn2Click(TObject *Sender);
    void __fastcall cbxViewChange(TObject *Sender);
    void __fastcall BitBtn3Click(TObject *Sender);
	void __fastcall rtbTextingChange(TObject *Sender);
    void __fastcall BitBtn4Click(TObject *Sender);
    void __fastcall BitBtn8Click(TObject *Sender);
    void __fastcall fdgFind(TObject *Sender);
    void __fastcall BitBtn10Click(TObject *Sender);
    void __fastcall nudFromChange(TObject *Sender);
    void __fastcall nudToChange(TObject *Sender);
    void __fastcall MyIDEditChange(TObject *Sender);
    void __fastcall BitBtn5Click(TObject *Sender);
	void __fastcall RecMesBoxClick(TObject *Sender);
	void __fastcall BitBtn11Click(TObject *Sender);
	void __fastcall btnDeleteMClick(TObject *Sender);
	void __fastcall CSpinEdit1Change(TObject *Sender);
	void __fastcall CSpinEdit2Change(TObject *Sender);
    void __fastcall Button4Click(TObject *Sender);
    void __fastcall BitBtn6Click(TObject *Sender);
	void __fastcall BitBtn12Click(TObject *Sender);
	void __fastcall BitBtn13Click(TObject *Sender);
	void __fastcall BitBtn15Click(TObject *Sender);
	void __fastcall BitBtn16Click(TObject *Sender);
	void __fastcall Button6Click(TObject *Sender);
	void __fastcall BitBtn18Click(TObject *Sender);
	void __fastcall grpahTimer(TObject *Sender);
	void __fastcall BitBtn19Click(TObject *Sender);
	void __fastcall BitBtn17Click(TObject *Sender);
	void __fastcall ComboBox2Change(TObject *Sender);
	void __fastcall CSpinEdit3Change(TObject *Sender);
	void __fastcall CSpinEdit4Change(TObject *Sender);
    void __fastcall CSpinEdit6Change(TObject *Sender);
	void __fastcall BitBtn20Click(TObject *Sender);
    void __fastcall BitBtn22Click(TObject *Sender);
    void __fastcall BitBtn21Click(TObject *Sender);
	void __fastcall BitBtn1Click(TObject *Sender);
	void __fastcall Button5Click(TObject *Sender);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall CheckListBox1Click(TObject *Sender);
    //void __fastcall ckbcomopenClick(TObject *Sender);
private:	// User declarations
unsigned long ulSize;
	bool FAllocated;
    bool FRecording;
    bool FPlaying;
	HWAVEIN hwi;
    HWAVEOUT hwo;    
	HGLOBAL HData, HWaveHdr;
    LPSTR lpData;
	LPWAVEHDR lpWaveHdr;

    void __fastcall MMWimData(TMessage &Msg);
    void __fastcall MMWomDone(TMessage &Msg);

public:		// User declarations
	__fastcall TForm1(TComponent* Owner);
	__fastcall ~TForm1();
BEGIN_MESSAGE_MAP
	MESSAGE_HANDLER(MM_WIM_DATA, TMessage, MMWimData)
	MESSAGE_HANDLER(MM_WOM_DONE, TMessage, MMWomDone)
END_MESSAGE_MAP(TForm)
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
