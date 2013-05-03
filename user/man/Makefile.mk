all: 	evlview.1.gz evlsend.1.gz evlconfig.1.gz evlnotify.1.gz evlremote.1.gz\
	evlquery.1.gz evlfacility.1.gz evltc.1.gz evlogmgr.1.gz evlog.1.gz \
	evlgentmpls.1.gz

%.1.gz:           %.1
	gzip -c $< > $@

clean:
	rm -f *.gz

clobber: clean
