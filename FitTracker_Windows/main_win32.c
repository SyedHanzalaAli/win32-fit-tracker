/*
 * FitTracker — Simple Fitness Guide  (Win32 / C)
 * Build: gcc main_win32.c -o FitTracker.exe -mwindows -lcomctl32 -O2
 */
#define WINVER       0x0501
#define _WIN32_WINNT 0x0501
#define _WIN32_IE    0x0501

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* ── Palette ─────────────────────────────────────────── */
#define C_BG      RGB(13,  17,  30)
#define C_PANEL   RGB(20,  26,  50)
#define C_TILE    RGB(24,  32,  62)
#define C_TILE_HV RGB(32,  42,  80)
#define C_ACCENT  RGB(0,   201, 150)
#define C_TEXT    RGB(225, 230, 245)
#define C_DIM     RGB(140, 148, 175)
#define C_EDIT    RGB(10,  14,  34)
#define C_HEADER  RGB(8,   10,  22)
#define C_ERR     RGB(255, 90,  90)
#define C_OK      RGB(0,   201, 150)

/* ── Control IDs ─────────────────────────────────────── */
/* Setup */
#define ID_SG      101
#define ID_SGL     102
#define ID_START   103
/* Dashboard tiles (owner-draw) */
#define ID_TILE_D  201
#define ID_TILE_W  202
#define ID_TILE_T  203
#define ID_CHNG    204
/* Plan view */
#define ID_PBACK   301
#define ID_PTEXT   302
/* Tracker */
#define ID_TBACK   401
#define ID_DIET_HDR_LBL 402
#define ID_CHK_D   403
#define ID_DNOTE_L 404
#define ID_DNOTE   405
#define ID_CALC_L  406
#define ID_CALC    407
#define ID_CALB_L  408
#define ID_CALB    409
#define ID_WORK_HDR_LBL 410
#define ID_CHK_W   411
#define ID_WNOTE_L 412
#define ID_WNOTE   413
#define ID_SAVE    414
#define ID_SAVEMSG 415
#define ID_HIST_L  416
#define ID_HIST    417

/* ── App state ───────────────────────────────────────── */
#define CFG_FILE  "config.txt"
#define LOG_FILE  "progress.csv"

typedef enum { SCR_SETUP, SCR_DASH, SCR_PLAN, SCR_TRACK } Screen;
static Screen gScr      = SCR_SETUP;
static int    gPlanDiet = 1;   /* 1=diet plan, 0=workout plan */

static char gGender[16] = "Male";
static char gGoal[32]   = "fatloss";

static int hasDiet(void){ return !strcmp(gGoal,"fatloss")  || !strcmp(gGoal,"both"); }
static int hasWork(void){ return !strcmp(gGoal,"musclegain")|| !strcmp(gGoal,"both"); }
static int hasBoth(void){ return !strcmp(gGoal,"both"); }

/* ── GDI objects ─────────────────────────────────────── */
static HBRUSH hBrBg, hBrPanel, hBrEdit, hBrHeader, hBrAccent;
static HFONT  hFBig,  /* 26pt – screen title          */
              hFOpt,  /* 20pt – tile / button labels  */
              hFBold, /* 17pt – section headers       */
              hFNorm, /* 15pt – labels                */
              hFSub,  /* 13pt – subtitles / dim       */
              hFMono; /* 13pt – plan / history text   */

/* ── Window ──────────────────────────────────────────── */
static HWND hMain;

/* Setup controls */
static HWND hSG, hSGL, hBtnStart;
static HWND hSTitle, hSSub, hSGL_L, hSG_L;

/* Dashboard controls */
static HWND hDProfile, hDChange;
static HWND hTileD, hTileW, hTileT;
static HWND hDSub;

/* Plan controls */
static HWND hPBack, hPTitle, hPText;

/* Tracker controls */
static HWND hTBack, hTTitle, hTDate;
static HWND hDietHdrLbl, hChkDiet, hDNoteL, hDNote, hCalCL, hCalC, hCalBL, hCalB;
static HWND hWorkHdrLbl, hChkWork, hWNoteL, hWNote;
static HWND hBtnSave, hSaveMsg;
static HWND hHistL, hHist;

/* ═══════════════════════════════════════════════════════
 *  CONFIG  (persist gender+goal between sessions)
 * ═══════════════════════════════════════════════════════ */
static void LoadConfig(void){
    FILE *f = fopen(CFG_FILE,"r");
    if(!f) return;
    char g[16]="", gl[32]="";
    if(fscanf(f,"%15[^,],%31[^\n]",g,gl)==2){
        snprintf(gGender,16,"%s",g);
        snprintf(gGoal,  32,"%s",gl);
    }
    fclose(f);
}
static void SaveConfig(void){
    FILE *f = fopen(CFG_FILE,"w");
    if(!f) return;
    fprintf(f,"%s,%s\n",gGender,gGoal);
    fclose(f);
}

/* ═══════════════════════════════════════════════════════
 *  PLAN FILES
 * ═══════════════════════════════════════════════════════ */
static void toCRLF(const char *in, char *out, int cap){
    int w=0;
    for(int i=0; in[i]&&w<cap-2; i++){
        if(in[i]=='\n'){ out[w++]='\r'; out[w++]='\n'; }
        else out[w++]=in[i];
    }
    out[w]='\0';
}
static void LoadPlan(HWND hEdit, const char *prefix){
    char fname[256];
    snprintf(fname,sizeof(fname),"%s%s%s.txt",prefix,gGoal,gGender);
    FILE *f=fopen(fname,"r");
    if(!f){
        char msg[300];
        snprintf(msg,sizeof(msg),
            "File not found: %s\r\n\r\n"
            "Make sure all .txt plan files are in the same folder as FitTracker.exe.",
            fname);
        SetWindowTextA(hEdit,msg);
        return;
    }
    char raw[8192]="", buf[16384]="", line[512];
    while(fgets(line,sizeof(line),f))
        strncat(raw,line,sizeof(raw)-strlen(raw)-1);
    fclose(f);
    toCRLF(raw,buf,sizeof(buf));
    SetWindowTextA(hEdit,buf);
    SendMessage(hEdit,EM_SETSEL,0,0);
    SendMessage(hEdit,EM_SCROLLCARET,0,0);
}

/* ═══════════════════════════════════════════════════════
 *  PROGRESS LOG
 * ═══════════════════════════════════════════════════════ */
static void LogProgress(int dietDone, int workDone,
                        const char *dNotes, const char *wNotes,
                        int calC, int calB)
{
    FILE *f=fopen(LOG_FILE,"a+");
    if(!f) return;
    fseek(f,0,SEEK_END);
    if(ftell(f)==0)
        fprintf(f,"date,diet_done,workout_done,diet_notes,workout_notes,cal_consumed,cal_burned\n");
    time_t t=time(NULL); struct tm *tm=localtime(&t);
    char date[32]; strftime(date,sizeof(date),"%Y-%m-%d",tm);
    /* strip commas from notes to keep CSV clean */
    char dn[256], wn[256];
    snprintf(dn,sizeof(dn),"%s",dNotes);
    snprintf(wn,sizeof(wn),"%s",wNotes);
    for(int i=0;dn[i];i++) if(dn[i]==',') dn[i]=' ';
    for(int i=0;wn[i];i++) if(wn[i]==',') wn[i]=' ';
    fprintf(f,"%s,%d,%d,%s,%s,%d,%d\n",date,dietDone,workDone,dn,wn,calC,calB);
    fclose(f);
}

static void LoadHistory(HWND hEdit){
    FILE *f=fopen(LOG_FILE,"r");
    if(!f){
        SetWindowTextA(hEdit,"No progress logged yet. Start logging above!");
        return;
    }
    char buf[8192]="", line[512];
    fgets(line,sizeof(line),f); /* skip header */

    /* Read all rows, keep last 10 */
    char rows[10][512]; int cnt=0;
    while(fgets(line,sizeof(line),f)){
        if(strlen(line)<5) continue;
        int idx=cnt<10?cnt:9;
        if(cnt>=10) memmove(rows[0],rows[1],sizeof(char)*9*512);
        strncpy(rows[idx<10?idx:9],line,511);
        if(cnt<10) cnt++;
    }
    fclose(f);

    strcat(buf,"Date         Diet  Workout  Cal.In  Cal.Out  Notes\r\n");
    strcat(buf,"------------------------------------------------------\r\n");
    for(int i=0;i<cnt;i++){
        char date[16],dn[128],wn[128];
        int dd=0,wd=0,cc=0,cb=0;
        if(sscanf(rows[i],"%15[^,],%d,%d,%127[^,],%127[^,],%d,%d",
                  date,&dd,&wd,dn,wn,&cc,&cb)==7){
            char row[256];
            snprintf(row,sizeof(row),"%-13s%-6s%-9s%-8d%-9d%s\r\n",
                date,
                dd?"Yes":"No",
                wd?"Yes":"No",
                cc,cb,
                hasBoth() ? "" : (hasDiet()?dn:wn));
            strncat(buf,row,sizeof(buf)-strlen(buf)-1);
        }
    }
    SetWindowTextA(hEdit,buf);
    SendMessage(hEdit,EM_SETSEL,0,0);
    SendMessage(hEdit,EM_SCROLLCARET,0,0);
}

/* ═══════════════════════════════════════════════════════
 *  GOAL / GENDER helpers
 * ═══════════════════════════════════════════════════════ */
static const char *GoalLabel(const char *g){
    if(!strcmp(g,"fatloss"))   return "Fat Loss";
    if(!strcmp(g,"musclegain"))return "Muscle Gain";
    return "Both";
}
static const char *GoalKey(int idx){
    if(idx==1) return "musclegain";
    if(idx==2) return "both";
    return "fatloss";
}
static int GoalIdx(const char *g){
    if(!strcmp(g,"musclegain")) return 1;
    if(!strcmp(g,"both"))       return 2;
    return 0;
}

/* ═══════════════════════════════════════════════════════
 *  SHOW / HIDE helpers
 * ═══════════════════════════════════════════════════════ */
static void Vis(HWND h, int show){
    if(h) ShowWindow(h, show?SW_SHOW:SW_HIDE);
}
static void HideAll(void){
    HWND all[]={
        hSG,hSGL,hBtnStart,hSTitle,hSSub,hSGL_L,hSG_L,
        hDProfile,hDChange,hTileD,hTileW,hTileT,hDSub,
        hPBack,hPTitle,hPText,
        hTBack,hTTitle,hTDate,
        hDietHdrLbl,hChkDiet,hDNoteL,hDNote,hCalCL,hCalC,hCalBL,hCalB,
        hWorkHdrLbl,hChkWork,hWNoteL,hWNote,
        hBtnSave,hSaveMsg,hHistL,hHist
    };
    for(int i=0;i<(int)(sizeof(all)/sizeof(all[0]));i++)
        Vis(all[i],0);
}

/* ── Lay out and show tracker controls based on gGoal ── */
static void LayoutTracker(void){
    /* always hide everything first */
    HWND trk[]={
        hDietHdrLbl,hChkDiet,hDNoteL,hDNote,hCalCL,hCalC,hCalBL,hCalB,
        hWorkHdrLbl,hChkWork,hWNoteL,hWNote,
        hBtnSave,hSaveMsg,hHistL,hHist
    };
    for(int i=0;i<(int)(sizeof(trk)/sizeof(trk[0]));i++) Vis(trk[i],0);

    int lx=15, lw=870;   /* default: single column, full width */
    int dietY=70, workY=70;

    if(hasBoth()){
        /* two side-by-side columns */
        lx=15; lw=415;
        dietY=70; workY=70;   /* both start at same Y, different X */
    }

    if(hasDiet()){
        MoveWindow(hDietHdrLbl, lx,    dietY,      lw, 28, TRUE);
        MoveWindow(hChkDiet,    lx,    dietY+38,   lw, 28, TRUE);
        MoveWindow(hDNoteL,     lx,    dietY+76,   lw, 22, TRUE);
        MoveWindow(hDNote,      lx,    dietY+100,  lw, 65, TRUE);
        MoveWindow(hCalCL,      lx,    dietY+175,  200,22, TRUE);
        MoveWindow(hCalC,       lx+205,dietY+172,  100,28, TRUE);
        MoveWindow(hCalBL,      lx+315,dietY+175,  180,22, TRUE);
        MoveWindow(hCalB,       lx+495,dietY+172,  100,28, TRUE);
        Vis(hDietHdrLbl,1); Vis(hChkDiet,1);
        Vis(hDNoteL,1);     Vis(hDNote,1);
        Vis(hCalCL,1);      Vis(hCalC,1);
        Vis(hCalBL,1);      Vis(hCalB,1);
    }

    int wx = hasBoth() ? lx+440 : lx;
    int botY;   /* Y after all input sections */

    if(hasWork()){
        MoveWindow(hWorkHdrLbl, wx, workY,      lw, 28, TRUE);
        MoveWindow(hChkWork,    wx, workY+38,   lw, 28, TRUE);
        MoveWindow(hWNoteL,     wx, workY+76,   lw, 22, TRUE);
        MoveWindow(hWNote,      wx, workY+100,  lw, 65, TRUE);
        Vis(hWorkHdrLbl,1); Vis(hChkWork,1);
        Vis(hWNoteL,1);     Vis(hWNote,1);
        botY = workY+175;
    } else {
        botY = dietY+215;
    }

    /* Save button + message */
    int saveX=300, saveW=300;
    MoveWindow(hBtnSave,  saveX, botY,     saveW,44, TRUE);
    MoveWindow(hSaveMsg,  saveX, botY+52,  saveW,24, TRUE);
    Vis(hBtnSave,1); Vis(hSaveMsg,1);

    /* History section */
    int histY = botY+88;
    int histH = 650 - histY - 6;
    if(histH<80) histH=80;
    MoveWindow(hHistL, 15, histY,    870, 24,    TRUE);
    MoveWindow(hHist,  15, histY+28, 870, histH, TRUE);
    Vis(hHistL,1); Vis(hHist,1);
}

/* ═══════════════════════════════════════════════════════
 *  SCREEN TRANSITIONS
 * ═══════════════════════════════════════════════════════ */
static void ShowScreen(Screen s){
    HideAll();
    gScr=s;

    if(s==SCR_SETUP){
        Vis(hSTitle,1); Vis(hSSub,1);
        Vis(hSG_L,1);   Vis(hSG,1);
        Vis(hSGL_L,1);  Vis(hSGL,1);
        Vis(hBtnStart,1);
        SetFocus(hSG);

    } else if(s==SCR_DASH){
        char prof[80];
        snprintf(prof,sizeof(prof),"  %s  |  %s",gGender,GoalLabel(gGoal));
        SetWindowTextA(hDProfile,prof);
        Vis(hDProfile,1); Vis(hDChange,1);
        Vis(hTileD,1);    Vis(hTileW,1); Vis(hTileT,1);
        Vis(hDSub,1);

    } else if(s==SCR_PLAN){
        const char *prefix = gPlanDiet ? "diet_" : "workout_";
        const char *label  = gPlanDiet ? "7-Day Diet Plan" : "7-Day Workout Plan";
        SetWindowTextA(hPTitle,label);
        LoadPlan(hPText,prefix);
        Vis(hPBack,1); Vis(hPTitle,1); Vis(hPText,1);

    } else if(s==SCR_TRACK){
        /* date label */
        time_t t=time(NULL); struct tm *tm=localtime(&t);
        char date[64]; strftime(date,sizeof(date),"Today:  %A, %d %B %Y",tm);
        SetWindowTextA(hTDate,date);
        /* clear inputs */
        SendMessage(hChkDiet,BM_SETCHECK,BST_UNCHECKED,0);
        SendMessage(hChkWork,BM_SETCHECK,BST_UNCHECKED,0);
        SetWindowTextA(hDNote,""); SetWindowTextA(hWNote,"");
        SetWindowTextA(hCalC,""); SetWindowTextA(hCalB,"");
        SetWindowTextA(hSaveMsg,"");
        Vis(hTBack,1); Vis(hTTitle,1); Vis(hTDate,1);
        LayoutTracker();
        LoadHistory(hHist);
    }
    InvalidateRect(hMain,NULL,TRUE);
}

/* ═══════════════════════════════════════════════════════
 *  CONTROL FACTORY
 * ═══════════════════════════════════════════════════════ */
static HWND C(const char *cls,const char *txt,DWORD sty,
              int x,int y,int w,int h,UINT id){
    return CreateWindowExA(0,cls,txt,WS_CHILD|sty,x,y,w,h,
        hMain,(HMENU)(UINT_PTR)id,
        (HINSTANCE)GetWindowLongPtrA(hMain,GWLP_HINSTANCE),NULL);
}
static void F(HWND h,HFONT f){ SendMessage(h,WM_SETFONT,(WPARAM)f,TRUE); }

/* ── Build all controls once at WM_CREATE ── */
static void CreateAllControls(void){

    /* ── SETUP ─────────────────────────────────────────── */
    hSTitle   = C("STATIC","FitTracker",     SS_CENTER,175, 90,550,55,0); F(hSTitle,hFBig);
    hSSub     = C("STATIC","Your personal fitness guide",
                                             SS_CENTER,175,150,550,26,0); F(hSSub,hFSub);
    hSG_L     = C("STATIC","Select your Gender:",
                                             SS_LEFT,  195,205,510,28,0); F(hSG_L,hFBold);
    hSG       = C(WC_COMBOBOXA,"",CBS_DROPDOWNLIST|WS_VSCROLL,
                                                   195,237,510,220,ID_SG); F(hSG,hFOpt);
    SendMessageA(hSG,CB_ADDSTRING,0,(LPARAM)"Male");
    SendMessageA(hSG,CB_ADDSTRING,0,(LPARAM)"Female");
    SendMessageA(hSG,CB_SETCURSEL,0,0);

    hSGL_L    = C("STATIC","Select your Goal:",
                                             SS_LEFT,  195,300,510,28,0); F(hSGL_L,hFBold);
    hSGL      = C(WC_COMBOBOXA,"",CBS_DROPDOWNLIST|WS_VSCROLL,
                                                   195,332,510,220,ID_SGL); F(hSGL,hFOpt);
    SendMessageA(hSGL,CB_ADDSTRING,0,(LPARAM)"Fat Loss");
    SendMessageA(hSGL,CB_ADDSTRING,0,(LPARAM)"Muscle Gain");
    SendMessageA(hSGL,CB_ADDSTRING,0,(LPARAM)"Both");
    SendMessageA(hSGL,CB_SETCURSEL,0,0);

    hBtnStart = C("BUTTON","Get Started",BS_OWNERDRAW, 225,428,450,56,ID_START);
    F(hBtnStart,hFOpt);

    /* ── DASHBOARD ─────────────────────────────────────── */
    hDProfile = C("STATIC","",SS_LEFT|SS_CENTERIMAGE,15,0,750,54,0);
    F(hDProfile,hFNorm);
    hDChange  = C("BUTTON","Change Profile",BS_PUSHBUTTON,790,12,95,30,ID_CHNG);
    F(hDChange,hFSub);
    hDSub     = C("STATIC","Choose an option below:",
                            SS_CENTER,0,70,900,26,0); F(hDSub,hFNorm);

    /* 3 owner-draw tile buttons */
    hTileD = C("BUTTON","",BS_OWNERDRAW,35, 115,255,220,ID_TILE_D);
    hTileW = C("BUTTON","",BS_OWNERDRAW,323,115,255,220,ID_TILE_W);
    hTileT = C("BUTTON","",BS_OWNERDRAW,611,115,255,220,ID_TILE_T);

    /* ── PLAN VIEW ─────────────────────────────────────── */
    hPBack  = C("BUTTON","< Back",BS_PUSHBUTTON,12,12,90,32,ID_PBACK); F(hPBack,hFNorm);
    hPTitle = C("STATIC","",SS_LEFT|SS_CENTERIMAGE,115,8,670,44,0);    F(hPTitle,hFBold);
    hPText  = C("EDIT","",WS_BORDER|ES_MULTILINE|ES_READONLY|WS_VSCROLL|ES_AUTOVSCROLL,
                15,62,870,578,ID_PTEXT); F(hPText,hFMono);

    /* ── TRACKER ───────────────────────────────────────── */
    hTBack  = C("BUTTON","< Back",BS_PUSHBUTTON,12,12,90,32,ID_TBACK); F(hTBack,hFNorm);
    hTTitle = C("STATIC","Progress Tracker",
                SS_LEFT|SS_CENTERIMAGE,115,8,360,44,0); F(hTTitle,hFBold);
    hTDate  = C("STATIC","",SS_RIGHT|SS_CENTERIMAGE,490,8,395,44,0);   F(hTDate,hFNorm);

    /* diet section controls (positioned by LayoutTracker) */
    hDietHdrLbl = C("STATIC","Diet Progress Today",SS_LEFT,0,0,100,28,ID_DIET_HDR_LBL);
    F(hDietHdrLbl,hFBold);
    hChkDiet = C("BUTTON","Completed diet today?",
                 BS_AUTOCHECKBOX,0,0,100,28,ID_CHK_D); F(hChkDiet,hFNorm);
    hDNoteL  = C("STATIC","Notes / Meals eaten:",SS_LEFT,0,0,100,22,ID_DNOTE_L); F(hDNoteL,hFSub);
    hDNote   = C("EDIT","",WS_BORDER|ES_MULTILINE|ES_AUTOVSCROLL,
                 0,0,100,65,ID_DNOTE); F(hDNote,hFNorm);
    hCalCL   = C("STATIC","Calories consumed:",SS_LEFT,0,0,200,22,ID_CALC_L);  F(hCalCL,hFSub);
    hCalC    = C("EDIT","",WS_BORDER|ES_AUTOHSCROLL|ES_NUMBER,0,0,100,28,ID_CALC); F(hCalC,hFNorm);
    hCalBL   = C("STATIC","Calories burned:",SS_LEFT,0,0,180,22,ID_CALB_L);  F(hCalBL,hFSub);
    hCalB    = C("EDIT","",WS_BORDER|ES_AUTOHSCROLL|ES_NUMBER,0,0,100,28,ID_CALB); F(hCalB,hFNorm);

    /* workout section controls */
    hWorkHdrLbl = C("STATIC","Workout Progress Today",SS_LEFT,0,0,100,28,ID_WORK_HDR_LBL);
    F(hWorkHdrLbl,hFBold);
    hChkWork = C("BUTTON","Completed workout today?",
                 BS_AUTOCHECKBOX,0,0,100,28,ID_CHK_W); F(hChkWork,hFNorm);
    hWNoteL  = C("STATIC","Notes / Exercises done:",SS_LEFT,0,0,100,22,ID_WNOTE_L); F(hWNoteL,hFSub);
    hWNote   = C("EDIT","",WS_BORDER|ES_MULTILINE|ES_AUTOVSCROLL,
                 0,0,100,65,ID_WNOTE); F(hWNote,hFNorm);

    /* save + history */
    hBtnSave = C("BUTTON","Save Today's Log",BS_OWNERDRAW,0,0,100,44,ID_SAVE);
    F(hBtnSave,hFBold);
    hSaveMsg = C("STATIC","",SS_CENTER,0,0,100,24,ID_SAVEMSG); F(hSaveMsg,hFSub);
    hHistL   = C("STATIC","Recent Progress (last 10 entries):",
                 SS_LEFT,0,0,100,24,ID_HIST_L); F(hHistL,hFSub);
    hHist    = C("EDIT","",WS_BORDER|ES_MULTILINE|ES_READONLY|WS_VSCROLL|ES_AUTOVSCROLL,
                 0,0,100,80,ID_HIST); F(hHist,hFMono);
}

/* ═══════════════════════════════════════════════════════
 *  DRAW TILE / BUTTON helper (WM_DRAWITEM)
 * ═══════════════════════════════════════════════════════ */
static void DrawTile(LPDRAWITEMSTRUCT d,
                     const char *head, const char *line1, const char *line2){
    BOOL sel = (d->itemState & ODS_SELECTED)!=0;
    COLORREF bg = sel ? C_TILE_HV : C_TILE;
    HBRUSH br = CreateSolidBrush(bg);
    FillRect(d->hDC, &d->rcItem, br); DeleteObject(br);

    /* accent top bar */
    RECT bar={d->rcItem.left,d->rcItem.top,d->rcItem.right,d->rcItem.top+5};
    br=CreateSolidBrush(C_ACCENT); FillRect(d->hDC,&bar,br); DeleteObject(br);

    /* subtle border */
    HPEN pen=CreatePen(PS_SOLID,1,RGB(35,45,85));
    SelectObject(d->hDC,pen);
    SelectObject(d->hDC,GetStockObject(NULL_BRUSH));
    Rectangle(d->hDC,d->rcItem.left,d->rcItem.top,d->rcItem.right,d->rcItem.bottom);
    DeleteObject(pen);

    SetBkMode(d->hDC,TRANSPARENT);

    /* heading */
    SelectObject(d->hDC,hFOpt);
    SetTextColor(d->hDC,C_TEXT);
    RECT tr={d->rcItem.left+16,d->rcItem.top+22,d->rcItem.right-8,d->rcItem.top+72};
    DrawTextA(d->hDC,head,-1,&tr,DT_LEFT|DT_VCENTER|DT_SINGLELINE);

    /* description lines */
    SelectObject(d->hDC,hFSub);
    SetTextColor(d->hDC,C_DIM);
    RECT dr={d->rcItem.left+16,d->rcItem.top+82,d->rcItem.right-8,d->rcItem.top+108};
    DrawTextA(d->hDC,line1,-1,&dr,DT_LEFT|DT_SINGLELINE);
    dr.top+=26; dr.bottom+=26;
    DrawTextA(d->hDC,line2,-1,&dr,DT_LEFT|DT_SINGLELINE);
}

static void DrawAccentBtn(LPDRAWITEMSTRUCT d, const char *label){
    BOOL sel=(d->itemState&ODS_SELECTED)!=0;
    COLORREF bg = sel ? RGB(0,160,120) : C_ACCENT;
    HBRUSH br=CreateSolidBrush(bg);
    FillRect(d->hDC,&d->rcItem,br); DeleteObject(br);
    SetBkMode(d->hDC,TRANSPARENT);
    SelectObject(d->hDC,hFOpt);
    SetTextColor(d->hDC,RGB(10,14,34));
    DrawTextA(d->hDC,label,-1,&d->rcItem,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
}

/* ═══════════════════════════════════════════════════════
 *  WINDOW PROCEDURE
 * ═══════════════════════════════════════════════════════ */
LRESULT CALLBACK WndProc(HWND hw,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){

    /* ── CREATE ── */
    case WM_CREATE:
        hMain=hw;
        hBrBg     = CreateSolidBrush(C_BG);
        hBrPanel  = CreateSolidBrush(C_PANEL);
        hBrEdit   = CreateSolidBrush(C_EDIT);
        hBrHeader = CreateSolidBrush(C_HEADER);
        hBrAccent = CreateSolidBrush(C_ACCENT);
        hFBig  = CreateFontA(26,0,0,0,FW_BOLD,  0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
        hFOpt  = CreateFontA(20,0,0,0,FW_BOLD,  0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
        hFBold = CreateFontA(17,0,0,0,FW_BOLD,  0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
        hFNorm = CreateFontA(15,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
        hFSub  = CreateFontA(13,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
        hFMono = CreateFontA(13,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Consolas");
        LoadConfig();
        CreateAllControls();
        /* pre-select saved gender/goal */
        SendMessageA(hSG, CB_SETCURSEL,
            strcmp(gGender,"Female")==0?1:0, 0);
        SendMessageA(hSGL,CB_SETCURSEL,GoalIdx(gGoal),0);
        ShowScreen(SCR_SETUP);
        return 0;

    /* ── PAINT ── */
    case WM_PAINT:{
        PAINTSTRUCT ps;
        HDC hdc=BeginPaint(hw,&ps);
        RECT rc; GetClientRect(hw,&rc);
        FillRect(hdc,&rc,hBrBg);
        if(gScr==SCR_SETUP){
            /* setup card */
            RECT card={170,65,730,512};
            HBRUSH b=CreateSolidBrush(C_PANEL);
            FillRect(hdc,&card,b); DeleteObject(b);
            RECT ab={170,65,730,70};
            FillRect(hdc,&ab,hBrAccent);
        } else {
            /* top header bar */
            RECT hdr={0,0,rc.right,54};
            FillRect(hdc,&hdr,hBrHeader);
            RECT al={0,53,rc.right,55};
            FillRect(hdc,&al,hBrAccent);
            /* app name on left of header */
            SetBkMode(hdc,TRANSPARENT);
            SetTextColor(hdc,C_ACCENT);
            SelectObject(hdc,hFBold);
            RECT nrc={12,8,210,46};
            DrawTextA(hdc,"FitTracker",-1,&nrc,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        }
        EndPaint(hw,&ps);
        return 0;
    }
    case WM_ERASEBKGND: return 1;

    /* ── THEMING ── */
    case WM_CTLCOLORSTATIC:{
        HDC hdc=(HDC)wp; HWND hc=(HWND)lp;
        SetBkMode(hdc,TRANSPARENT);
        if(hc==hDietHdrLbl||hc==hWorkHdrLbl||hc==hPTitle||hc==hTTitle||hc==hSTitle)
            SetTextColor(hdc,C_ACCENT);
        else if(hc==hSSub||hc==hDSub||hc==hDNoteL||hc==hWNoteL||
                hc==hCalCL||hc==hCalBL||hc==hHistL)
            SetTextColor(hdc,C_DIM);
        else if(hc==hSaveMsg)
            SetTextColor(hdc,C_OK);
        else
            SetTextColor(hdc,C_TEXT);
        if(gScr==SCR_DASH||gScr==SCR_PLAN||gScr==SCR_TRACK)
            return (LRESULT)hBrBg;
        return (LRESULT)hBrPanel;
    }
    case WM_CTLCOLOREDIT:{
        HDC hdc=(HDC)wp;
        SetTextColor(hdc,C_TEXT); SetBkColor(hdc,C_EDIT);
        return (LRESULT)hBrEdit;
    }
    case WM_CTLCOLORBTN:     return (LRESULT)hBrBg;
    case WM_CTLCOLORLISTBOX:{
        HDC hdc=(HDC)wp;
        SetTextColor(hdc,C_TEXT); SetBkColor(hdc,C_EDIT);
        return (LRESULT)hBrEdit;
    }
    case WM_CTLCOLORDLG: return (LRESULT)hBrBg;

    /* ── OWNER DRAW ── */
    case WM_DRAWITEM:{
        LPDRAWITEMSTRUCT d=(LPDRAWITEMSTRUCT)lp;
        switch(d->CtlID){
        case ID_TILE_D:
            DrawTile(d,"Diet Plan","View your personalized","7-day meal plan");
            return TRUE;
        case ID_TILE_W:
            DrawTile(d,"Workout Plan","View your personalized","7-day gym schedule");
            return TRUE;
        case ID_TILE_T:
            DrawTile(d,"Tracker","Log and review","your daily progress");
            return TRUE;
        case ID_START:
            DrawAccentBtn(d,"Get Started  \xBB");
            return TRUE;
        case ID_SAVE:
            DrawAccentBtn(d,"Save Today's Log");
            return TRUE;
        }
        break;
    }

    /* ── COMMANDS ── */
    case WM_COMMAND:{
        WORD id=LOWORD(wp);

        /* Setup: Get Started */
        if(id==ID_START){
            int gi =(int)SendMessageA(hSG, CB_GETCURSEL,0,0);
            int gli=(int)SendMessageA(hSGL,CB_GETCURSEL,0,0);
            if(gi <0) gi =0;
            if(gli<0) gli=0;
            snprintf(gGender,16,"%s",gi==0?"Male":"Female");
            snprintf(gGoal,  32,"%s",GoalKey(gli));
            SaveConfig();
            ShowScreen(SCR_DASH);
            break;
        }

        /* Dashboard: Change Profile */
        if(id==ID_CHNG){ ShowScreen(SCR_SETUP); break; }

        /* Dashboard tiles */
        if(id==ID_TILE_D){ gPlanDiet=1; ShowScreen(SCR_PLAN); break; }
        if(id==ID_TILE_W){ gPlanDiet=0; ShowScreen(SCR_PLAN); break; }
        if(id==ID_TILE_T){ ShowScreen(SCR_TRACK); break; }

        /* Plan: Back */
        if(id==ID_PBACK){ ShowScreen(SCR_DASH); break; }

        /* Tracker: Back */
        if(id==ID_TBACK){ ShowScreen(SCR_DASH); break; }

        /* Tracker: Save */
        if(id==ID_SAVE){
            int dd=(int)SendMessage(hChkDiet,BM_GETCHECK,0,0)==BST_CHECKED;
            int wd=(int)SendMessage(hChkWork,BM_GETCHECK,0,0)==BST_CHECKED;
            char dn[256]="", wn[256]="";
            char sc[32]="",  sb[32]="";
            if(hasDiet()){
                GetWindowTextA(hDNote,dn,sizeof(dn));
                GetWindowTextA(hCalC, sc,sizeof(sc));
                GetWindowTextA(hCalB, sb,sizeof(sb));
            }
            if(hasWork()) GetWindowTextA(hWNote,wn,sizeof(wn));
            int cc=atoi(sc), cb=atoi(sb);
            LogProgress(dd,wd,dn,wn,cc,cb);
            SetWindowTextA(hSaveMsg,"Saved successfully!");
            InvalidateRect(hSaveMsg,NULL,TRUE);
            LoadHistory(hHist);
            break;
        }
        break;
    }

    /* ── DESTROY ── */
    case WM_DESTROY:
        DeleteObject(hBrBg);    DeleteObject(hBrPanel);
        DeleteObject(hBrEdit);  DeleteObject(hBrHeader); DeleteObject(hBrAccent);
        DeleteObject(hFBig);    DeleteObject(hFOpt);
        DeleteObject(hFBold);   DeleteObject(hFNorm);
        DeleteObject(hFSub);    DeleteObject(hFMono);
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcA(hw,msg,wp,lp);
}

/* ═══════════════════════════════════════════════════════
 *  WINMAIN
 * ═══════════════════════════════════════════════════════ */
int WINAPI WinMain(HINSTANCE hI,HINSTANCE hP,LPSTR lC,int nS){
    (void)hP;(void)lC;
    InitCommonControls();

    WNDCLASSEXA wc; memset(&wc,0,sizeof(wc));
    wc.cbSize=sizeof(wc); wc.style=CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc=WndProc; wc.hInstance=hI;
    wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName="FitTracker";
    wc.hIcon=LoadIcon(NULL,IDI_APPLICATION);
    RegisterClassExA(&wc);

    RECT wr={0,0,900,660};
    AdjustWindowRect(&wr,WS_OVERLAPPEDWINDOW&~WS_THICKFRAME&~WS_MAXIMIZEBOX,FALSE);
    HWND hw=CreateWindowExA(0,"FitTracker","FitTracker - Fitness Guide",
        WS_OVERLAPPEDWINDOW&~WS_THICKFRAME&~WS_MAXIMIZEBOX,
        CW_USEDEFAULT,CW_USEDEFAULT,
        wr.right-wr.left,wr.bottom-wr.top,
        NULL,NULL,hI,NULL);

    ShowWindow(hw,nS); UpdateWindow(hw);
    MSG m;
    while(GetMessage(&m,NULL,0,0)){
        if(!IsDialogMessage(hw,&m)){
            TranslateMessage(&m); DispatchMessage(&m);
        }
    }
    return (int)m.wParam;
}
