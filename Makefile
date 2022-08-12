DEBUG = -g

CFLAGS = -c -fPIC ${DEBUG} -Wall -Werror -Wextra -std=c99 -pedantic
LDFLAGS = ${TRAY_LDFLAGS}


TRAY_CFLAGS = -c -fPIC ${DEBUG} `pkg-config --cflags appindicator-0.1`
TRAY_LDFLAGS = `pkg-config --libs appindicator-0.1`

ideapad-acpi-tray: tray_linux.o main.o
	gcc -o $@ ${LDFLAGS} $^
	sudo chown root:root $@
	sudo chmod u+s $@

tray_linux.o: tray_linux.c
	$(CC) ${TRAY_CFLAGS} -o $@ $<

main.o: main.c
	$(CC) ${CFLAGS} -o $@ $<


clean:
	$(RM) tray_linux.o main.o ideapad-acpi-tray