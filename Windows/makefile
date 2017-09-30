WWPKG16=winww16.exe winww.hlp winww.ini
WWPKG32=winww32.exe winww.hlp winww.ini
XWPKG16=winxw16.exe winxw.hlp winxw.ini
XWPKG32=winxw32.exe winxw.hlp winxw.ini
#DEMPKG16=$(WWPKG16) $(XWPKG16) wwdemo.dic thes.dat dll\ctl3dv2.dll README.TXT
#DEMPKG32=$(WWPKG32) $(XWPKG32) wwdemo.dic thes.dat dll\ctl3d32.dll README.TXT
DEMPKG16=winww16.exe winxw16.exe *.hlp *.ini wwdemo.dic thes.dat dll\ctl3dv2.dll README.TXT
DEMPKG32=winww32.exe winxw32.exe *.hlp *.ini wwdemo.dic thes.dat dll\ctl3d32.dll README.TXT
REGPKG16=wwbig.dic wwmed.dic reg.dat README2.TXT
REGPKG32=wwbig.dic wwmed.dic reg.dat README2.TXT
#TEX2RTF=e:\wx\utils\tex2rtf\src\tex2rtf.exe
TEX2RTF=d:\utils\tex2rtf.exe

default: win32 help shield

all: demo regzip 

all16: demo16 regzip16 help
 
all32: demo32 regzip32 help

demo: demo16 demo32 help

help: winww.hlp winxw.hlp

winww.hlp: winww.tex
	-$(TEX2RTF) winww.tex winww.rtf -winhelp
	hc winww.hpj

winxw.hlp: winxw.tex
	-$(TEX2RTF) winxw.tex winxw.rtf -winhelp
	hc winxw.hpj

demo16: win16 d16zip

d16zip:
	-tdstrip winww16.exe
	-tdstrip winxw16.exe
	-del ww16.zip
	copy c:\windows\system\ctl3dv2.dll .
	zip -upr -n .dic ww16 $(DEMPKG16)
	del ctl3dv2.dll
	copy /b d:\utils\unzipsfx.exe+ww16.zip ww16.exe
	zip -A ww16.exe

demo32: win32 d32zip

d32zip:
	-tdstrp32 winww32.exe
	-tdstrp32 winxw32.exe
	-del ww32.zip
	copy c:\windows\system\ctl3d32.dll .
	zip -upr -n .dic ww32 $(DEMPKG32)
	del ctl3d32.dll
	copy /b d:\utils\unzipsfx.exe+ww32.zip ww32.exe
	zip -A ww32.exe

reg16: preship makedict.exe reg.h regzip16

reg32: preship makedict.exe reg.h regzip32

regzip: regzip16 regzip32

regzip16: rzip16
#regzip16: dict16 rzip16

regzip32: rzip32
#regzip32: dict32 rzip32

rzip16:
	-tdstrip windic16.exe
	-del wwreg.zip
	zip -u -n .dic wwreg $(REGPKG16)
	copy /b d:\utils\unzipsfx.exe+wwreg.zip wwreg16.exe
	zip -A wwreg16.exe

rzip32:
	-tdstrp32 windic32.exe
	-del wwreg.zip
	zip -u -n .dic wwreg $(REGPKG32)
	copy /b d:\utils\unzipsfx.exe+wwreg.zip wwreg32.exe
	zip -A wwreg32.exe

preship:
	-del reg.dat
	-del reg.h
	-del xwordreg.h
	b regname.h

reg.h: buildreg.c version.h regname.h
	$(CC) $(CFLAGS) buildreg.c
	buildreg serials reg.h
	del buildreg.exe

makedict: makedict.exe

makedict.exe: makedict.c
	$(CC) $(LFLAGS) makedict.c

makethes: makethes.exe

makethes.exe: makethes.cc
	$(CC) $(LFLAGS) -P makethes.cc

win16: clean16
	make -f makefile.bcc

win32: clean32
	make -f makefile.b32

#dict16:
#	make -f makefile.bcc cleandic16
#	make -f makefile.bcc windic16.exe

#dict32:
#	make -f makefile.b32 cleandic32
#	make -f makefile.b32 windic32.exe

clean: clean16 clean32

clean16:
	make -f makefile.bcc clean

clean32:
	make -f makefile.b32 clean

shield:
	make -f makefile.b32 shield


