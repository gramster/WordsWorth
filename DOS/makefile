RSE	= rse
CC	= $(RSE) bcc
CFLAGS	= -ml -v -M -DBIGDICT
LFLAGS	= -ml -v -M -ls
DICT	= master.big
TAIL	= >>ww.err
OBJDIR	= .
STRIP	= tdstrip
PACK	= lzexe
CCLIB	= c:\bc\lib
DFLIB	= c:\local\lib

WWOBJ	= $(OBJDIR)/main.obj $(OBJDIR)/move.obj $(OBJDIR)/screen.obj $(OBJDIR)/dict.obj $(OBJDIR)/config.obj $(OBJDIR)/record.obj
XWORDOBJ= $(OBJDIR)/xword.obj $(OBJDIR)/dict.obj
WWDEMOBJ= $(OBJDIR)/main.obj $(OBJDIR)/demmove.obj $(OBJDIR)/demscreen.obj $(OBJDIR)/demdict.obj $(OBJDIR)/config.obj $(OBJDIR)/record.obj
XWORDEMOBJ= $(OBJDIR)/xworddem.obj $(OBJDIR)/demdict.obj
WWOBJ2	= main.obj move.obj screen.obj dict.obj config.obj record.obj
WWDEMOBJ2= main.obj demmove.obj demscreen.obj demdict.obj config.obj record.obj
XWORDOBJ2= xword.obj dict.obj
XWORDEMOBJ2= xworddem.obj demdict.obj

default: cleanerr buildict.exe readkey.exe wwdem.exe

real: ww

ship: prepare zipup

shipa: prepare packagea

shipb: prepare packageb

prepare: preship cleanerr readkey.exe makedict.exe reg.h ww setup install xsquash postmake

xprep: preship cleanerr readkey.exe makedict.exe xwordreg.h xword xsquash

packagea: zipup zipdem transfera

packageb: zipup zipdem transferb

packaged: zipup zipdem transferd

demb: zipdem trdemb

install: install.exe

postmake: 
	echo "Make OK"

zipup:
	ren ww.cfg wwcfg
	-del wwship.zip
	-pkzip -u wwship ww.exe xword.exe setup.exe rebuild.bat buildict.exe *.def *.cfg
	-pkzip -u wwship ww.doc xword.doc wwbig.dic wwmed.dic
	-pkzip -u wwship cryptic1.brd cryptic2.brd cryptic3.brd
	ren wwcfg ww.cfg

transfera:
	pause Place a blank formatted disk in drive A:
	copy wwship.zip a:/v
	copy c:\utils\unzip.exe a:/v
	copy readme a:/v
	copy readme.bat a:/v
	copy install.exe a:/v

transferb:
	pause Place a blank formatted disk in drive B:
	copy wwship.zip b:/v
	copy c:\utils\unzip.exe b:/v
	copy readme b:/v
	copy readme.bat b:/v
	copy install.exe b:/v

transferd:
	-mkdir d:\ww
	copy wwship.zip d:\ww/v
	copy j:\utils\unzip.exe d:\ww/v
	copy readme d:/v
	copy readme.bat d:/v
	copy install.exe d:\ww/v
	copy wwdem.zip d:\ww/v

trdemb:
	pause Place a blank formatted disk in drive B:
	copy j:\utils\unzip.exe b:/v
	copy install.exe b:/v
	copy readme b:/v
	copy readme.bat b:/v
	copy wwdem.zip b:/v

zipdem: wwdem.exe xworddem.exe setup install.exe
	ren ww.exe wwfull.exe
	ren wwdem.exe ww.exe
	ren xword.exe xwfull.exe
	ren xworddem.exe xword.exe
	ren ww.cfg wwcfg
	ls -l ww.exe xword.exe setup.exe *.def *.cfg ww.doc xword.doc *.dem > filelist.dem
	-del wwdem.zip
	pkzip -u wwdem demo.brd ww.exe xword.exe setup.exe *.def *.cfg ww.doc xword.doc
	pkzip -u wwdem wwdict.dem filelist.dem
	ren wwcfg ww.cfg
	ren ww.exe wwdem.exe
	ren wwfull.exe ww.exe
	ren xword.exe xworddem.exe
	ren xwfull.exe xword.exe

zipsrc:
	pkzip -u wwsrc *.c *.cpp *.h *.def *.cfg makefile *.chr *.lib *.doc versions serial*.* todo

preship:
	del reg.h
	del xwordreg.h
	b regname.h

all : cleanerr makedict.exe reg.h ww wwdem.exe xword.exe

cleanerr:
	del ww.err

debug: ww.exe
	copy $(OBJDIR)\main.exe wwdebug.exe
	
dembug: wwdem.exe
	copy $(OBJDIR)\main.exe wwdebug.exe
	
reg.h: buildreg.c version.h regname.h
	$(CC) $(CFLAGS) buildreg.c $(TAIL)
	buildreg serials reg.h
	del buildreg.exe

xwordreg.h: buildreg.c version.h regname.h
	$(CC) $(CFLAGS) buildreg.c $(TAIL)
	buildreg xserials xwordreg.h
	del buildreg.exe

ww.bug: $(WWOBJ)
	$(CC) $(CFLAGS) $(WWOBJ2) graphics.lib bgif.lib $(TAIL)
	copy $(OBJDIR)\main.exe ww.exe

ww: ww.exe
	$(STRIP) ww.exe
	-$(PACK) ww.exe

ww.exe: $(WWOBJ)
	$(CC) $(LFLAGS) $(WWOBJ2) graphics.lib bgif.lib $(TAIL)
	copy $(OBJDIR)\main.exe ww.exe

wwdem.exe: $(WWDEMOBJ)
	$(CC) $(LFLAGS) $(WWDEMOBJ2) graphics.lib bgif.lib $(TAIL)
	copy $(OBJDIR)\main.exe wwdem.exe
	$(STRIP) wwdem.exe
	-$(PACK) wwdem.exe

xworddem.exe: $(XWORDEMOBJ)
	$(CC) $(LFLAGS) $(XWORDEMOBJ2) $(TAIL)
	$(STRIP) xworddem.exe
	-$(PACK) xworddem.exe

xword: xword.exe

xsquash: xword.exe
	$(STRIP) xword
	-$(PACK) xword

xword.exe: $(XWORDOBJ)
	$(CC) $(LFLAGS) $(XWORDOBJ2) $(TAIL)

lookup: lookup.exe

lookup.exe: $(OBJDIR)/lookup.obj $(OBJDIR)/dict.obj
	$(CC) $(LFLAGS) $(OBJDIR)/lookup.obj $(OBJDIR)/dict.obj $(TAIL)

dumpdict: dumpdict.exe
	dumpdict $(DICT) > dump.dic

dumpdict.exe: $(OBJDIR)/dumpdict.obj $(OBJDIR)/dict.obj
	$(CC) $(LFLAGS) $(OBJDIR)/dumpdict.obj $(OBJDIR)/dict.obj $(TAIL)

makedict: makedict.exe

makedict.exe: makedict.c
	$(CC) $(LFLAGS) makedict.c $(TAIL)

buildict: buildict.exe

buildict.exe: buildict.c
	$(CC) $(LFLAGS) buildict.c $(TAIL)
	$(STRIP) buildict
	-$(PACK) buildict

readkey.exe: readkey.c
	$(CC) $(CFLAGS) readkey.c $(TAIL)

$(OBJDIR)/main.obj: ww.c ww.h
	$(CC) $(CFLAGS) -o$(OBJDIR)/main.obj -c ww.c $(TAIL)

$(OBJDIR)/screen.obj: screen.c ww.h reg.h version.h reg.h dict.h
	$(CC) $(CFLAGS) -o$(OBJDIR)/screen.obj -c -DREGISTERED screen.c $(TAIL)

$(OBJDIR)/demscreen.obj: screen.c ww.h reg.h version.h reg.h dict.h
	$(CC) $(CFLAGS) -c -o$(OBJDIR)/demscreen -DDEMO_VERSION screen.c $(TAIL)

$(OBJDIR)/dict.obj: dict.c dict.h version.h
	$(CC) -c $(CFLAGS) -o$(OBJDIR)/dict.obj dict.c $(TAIL)

$(OBJDIR)/demdict.obj: dict.c dict.h version.h
	$(CC) $(CFLAGS) -c -o$(OBJDIR)/demdict -DDEMO_VERSION dict.c $(TAIL)

$(OBJDIR)/move.obj: move.c ww.h dict.h
	$(CC) -c $(CFLAGS) -o$(OBJDIR)/move.obj move.c $(TAIL)

$(OBJDIR)/demmove.obj: move.c ww.h dict.h
	$(CC) $(CFLAGS) -c -o$(OBJDIR)/demmove -DDEMO_VERSION move.c $(TAIL)

$(OBJDIR)/config.obj: config.c ww.h dict.h
	$(CC) -c $(CFLAGS) -o$(OBJDIR)/config.obj config.c $(TAIL)

$(OBJDIR)/record.obj: record.c ww.h dict.h
	$(CC) -c $(CFLAGS) -o$(OBJDIR)/record.obj record.c $(TAIL)

$(OBJDIR)/xword.obj: xword.c dict.h xwordreg.h
	$(CC) $(CFLAGS) -o$(OBJDIR)/xword.obj -DREGISTERED -c xword.c $(TAIL)

$(OBJDIR)/xworddem.obj: xword.c dict.h xwordreg.h
	$(CC) $(CFLAGS) -o$(OBJDIR)/xworddem.obj -DDEMO_VERSION -c xword.c $(TAIL)

$(OBJDIR)/lookup.obj: lookup.c dict.h
	$(CC) -c $(CFLAGS) -o$(OBJDIR)/lookup.obj lookup.c $(TAIL)

setup : setup.exe
	$(STRIP) setup
	-$(PACK) setup

setup.exe : setup.obj $(DFLIB)\dflatpp.lib $(DFLIB)\commdlg.lib
	tlink /L$(DFLIB) $(CCLIB)\c0l setup.obj, setup, setup, dflatpp commdlg $(CCLIB)\cl

setup.obj: setup.cpp
    bcc -O1 -c -d -ml -Ic:\local\include\dflat setup.cpp

install.exe : install.obj $(DFLIB)\dflatpp.lib $(DFLIB)\commdlg.lib
	tlink /L$(DFLIB) $(CCLIB)\c0l install.obj, install, install, dflatpp commdlg $(CCLIB)\cl
	$(STRIP) install
	-$(PACK) install

install.obj: install.cpp
    bcc -O1 -c -d -ml  -Ic:\local\include\dflat install.cpp

ww.h: dict.h
	touch ww.h

.c.obj:
	$(CC) -c $(CFLAGS) $< $(TAIL)
