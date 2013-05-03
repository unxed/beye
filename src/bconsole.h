/**
 * @namespace   beye
 * @file        bconsole.h
 * @brief       This file included BEYE console functions description.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
 * @author      Mauro Giachero
 * @date        02.11.2007
 * @note        Added "ungotstring" function to enable inline assemblers
**/
#ifndef __BCONSOLE__H
#define __BCONSOLE__H

#include "libbeye/twin.h"

namespace beye {

    extern TWindow *MainWnd,*HelpWnd,*TitleWnd,*CritErrWnd;

    void         __FASTCALL__ initBConsole( unsigned long vio_flg,unsigned long twin_flg );
    void         __FASTCALL__ termBConsole( void );
    bool        __FASTCALL__ IsKbdTerminate( void );
    void         __FASTCALL__ CleanKbdTermSig( void );

    typedef void (__FASTCALL__ * pagefunc)(TWindow *win,const any_t**__obj,unsigned i__obj,unsigned total_obj);

    void         __FASTCALL__ CloseWnd(TWindow *w);
    TWindow *    __FASTCALL__ CrtDlgWnd(const char *,tAbsCoord,tAbsCoord);
    TWindow *    __FASTCALL__ CrtDlgWndnls(const char *,tAbsCoord,tAbsCoord);
    TWindow *    __FASTCALL__ CrtMnuWnd(const char *,tAbsCoord,tAbsCoord,tAbsCoord,tAbsCoord);
    TWindow *    __FASTCALL__ CrtMnuWndnls(const char *,tAbsCoord,tAbsCoord,tAbsCoord,tAbsCoord);
    TWindow *    __FASTCALL__ CrtLstWnd(const char *,tAbsCoord,tAbsCoord);
    TWindow *    __FASTCALL__ CrtLstWndnls(const char *,tAbsCoord,tAbsCoord);
    TWindow *    __FASTCALL__ CrtHlpWnd(const char *,tAbsCoord,tAbsCoord);
    TWindow *    __FASTCALL__ CrtHlpWndnls(const char *,tAbsCoord,tAbsCoord);
    TWindow *    __FASTCALL__ CreateEditor(tAbsCoord X1,tAbsCoord Y1,tAbsCoord X2,tAbsCoord Y2,unsigned flags);
    TWindow *    __FASTCALL__ WindowOpen(tAbsCoord X1,tAbsCoord Y1,tAbsCoord X2,tAbsCoord Y2,unsigned flags);
    void         __FASTCALL__ DisplayBox(char **names,unsigned nlist,const char *title);

/** Edit string styles */
enum {
    __ESS_ENABLEINSERT =0x0001U, /**< enable insert mode */
    __ESS_HARDEDIT     =0x0002U, /**< inform, that editing within hard multiline
					editor without insert mode */
    __ESS_WANTRETURN   =0x0004U, /**< return from routine after each pressed key
					need for contest depended painting. */
    __ESS_ASHEX        =0x0008U, /**< worked, as hexadecimal editor, i.e. insert
					space on each third position */
    __ESS_NOTUPDATELEN =0x0010U, /**< if attr & __ESS_ASHEX procedure not will
					update field *maxlength on each return */
    __ESS_FILLER_7BIT  =0x0020U, /**< Editor for Assemebler mode */
    __ESS_NON_C_STR    =0x0040U, /**< Notify editor about non-C string */
    __ESS_NOREDRAW     =0x8000U /**< Force no redraw string */
};
/** Arguments for (x,e)editstring:
   s         - pointer to a buffer with editing strings
   legal     - pointer to a legal character set (all if NULL)
   maxlength - pointer to maximal possible length of string
	       contains real length on return from routine
      y      - if __ESS_HARDEDIT y position within using window
	       else if __ESS_NON_C - real length of
	       string, i.e. enabled for input CHAR 0x00
      stx    - pointer to a x position within using window (0 - base)
	       contains last x position on return
	       if NULL position = 0
      attr   - attributes of the editor (See 'Edit string styles')
     undo    - if not NULL twUsedWin as UnDo buffer, i.e.
	       restore contest on CtrlBkSpace
     func    - if not NULL then called to display prompt string
*/
    int         __FASTCALL__ eeditstring(TWindow* w,char *s,const char *legal,
					unsigned *maxlength, unsigned y,
					unsigned *stx,unsigned attr,char *undo,
					void (*func)(void));
    int          __FASTCALL__ xeditstring(TWindow* w,char *s,const char *legal,
					unsigned maxlength, void(*func)(void));
    void         __FASTCALL__ ErrMessageBox(const char * text,const char * title);
    void         __FASTCALL__ WarnMessageBox(const char * text,const char * title);
    void         __FASTCALL__ errnoMessageBox(const char * text,const char * title,int __errno__);
    void         __FASTCALL__ ListBox(const char ** names,unsigned nlist,const char * title);
    void         __FASTCALL__ TMessageBox(const char * text,const char * title);
    void         __FASTCALL__ NotifyBox(const char * text,const char * title);
    int          __FASTCALL__ PageBox(unsigned width,unsigned height,const any_t** __obj,
				 unsigned nobj,pagefunc func);
    void         __FASTCALL__ MemOutBox(const char *user_msg);
    TWindow *    __FASTCALL__ PleaseWaitWnd( void );

    bool        __FASTCALL__ Get2DigitDlg(const char *title,const char *text,unsigned char *xx);
    bool        __FASTCALL__ Get16DigitDlg(const char *title,const char *text,char attr,
						unsigned long long int *xx);
    bool        __FASTCALL__ Get8DigitDlg(const char *title,const char *text,char attr,unsigned long *xx);

    bool        __FASTCALL__ GetStringDlg(char * buff,const char * title,const char *subtitle,
				     const char *prompt);
enum {
    GJDLG_FILE_TOP  =0x00000000UL,
    GJDLG_RELATIVE  =0x00000001UL,
    GJDLG_REL_EOF   =0x00000002UL,
    GJDLG_VIRTUAL   =0x00000003UL,
    GJDLG_PERCENTS  =0x00000004UL
};
    bool        __FASTCALL__ GetJumpDlg( __filesize_t * addr,unsigned long *flags);

enum {
    FSDLG_BINMODE   =0x00000000UL,
    FSDLG_ASMMODE   =0x00000001UL,
    FSDLG_NOCOMMENT =0x00000000UL,
    FSDLG_COMMENT   =0x00000002UL,
    FSDLG_STRUCTS   =0x00000004UL,
    FSDLG_NOMODES   =0x00000000UL,
    FSDLG_USEMODES  =0x80000000UL,

    FSDLG_BTNSMASK  =0x00000003UL, /**< 0=8bit 1=16bit 2=32bit 3=64bit */
    FSDLG_USEBITNS  =0x40000000UL
};
    bool        __FASTCALL__ GetFStoreDlg(const char *title,char *fname,
				     unsigned long *flags,
				     __filesize_t *start,
				     __filesize_t *end,
				     const char *prompt);
    bool        __FASTCALL__ GetInsDelBlkDlg(const char *title,__filesize_t *start,
					__fileoff_t *size);
enum {
    LB_SELECTIVE =0x01U,
    LB_SORTABLE  =0x02U,
    LB_USEACC    =0x04U,

    LB_ORD_DELIMITER =TWC_FL_BLK
};
    int          __FASTCALL__ CommonListBox(char** names,unsigned nlist,const char *title,
				      int acc,unsigned defsel);
    int          __FASTCALL__ SelBox(char** names,unsigned nlist,const char * title,
			       unsigned defsel);
    int          __FASTCALL__ SelBoxA(char** names,unsigned nlist,const char * title,
				unsigned defsel);
    int         __FASTCALL__ SelListBox(char** names,unsigned nlist,const char * title,
				   unsigned defsel);

    TWindow *    __FASTCALL__ PercentWnd(const char *text,const char *title);

			   /** return true - if can continue
				     false -  if user want terminate process */
    bool        __FASTCALL__ ShowPercentInWnd(TWindow *prcntwnd,unsigned n);

    int          __FASTCALL__ GetEvent(void (*)(void),int (*)(void),TWindow *);
    void         __FASTCALL__ PostEvent(int kbdcode);

    bool __FASTCALL__ _lb_searchtext(const char *str,const char *tmpl,
					 unsigned searchlen,const int *cache,
					 unsigned flg);
    void __FASTCALL__ __drawSinglePrompt(const char *prmt[]);

    bool __FASTCALL__ ungotstring(char *string);
} // namespace beye
#endif
