#define _XOPEN_SOURCE 600

#include <unistd.h>
#include <limits.h>
#include <fcntl.h>

#ifndef DO_NOT_SEARCH
#include <dirent.h>
#endif

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "config.h"
#include "tray.h"

const int IDEAPAD_ACPI_PATH_LEN = (sizeof(IDEAPAD_ACPI_PATH)/sizeof(*IDEAPAD_ACPI_PATH)-1);
const int IDEAPAD_DEVICE_PREFIX_LEN = (sizeof(IDEAPAD_DEVICE_PREFIX)/sizeof(*IDEAPAD_DEVICE_PREFIX)-1);

#define VALID_ID_CHARS "0123456789ABCDEF"

#define UNUSED __attribute__((unused))

#define FAILf(FMT, ...) \
	do { \
		fprintf(stderr, "error: " FMT "\n", __VA_ARGS__); \
		rc = 1; \
		goto failure; \
	} while (0)

#define FAIL(MSG) FAILf("%s", MSG)

#define FAIL_PERROR(MSG) FAILf(MSG ": %s", strerror(errno))

#define SNPRINTF_PATH(O, L, ...) \
	do { \
		if (snprintf(O, L, __VA_ARGS__) >= L) \
			FAIL("snprintf() possible truncation"); \
	} while (0)

static int drop_privilege(void) {
	int rc = 0;
	gid_t gid = 0;
	uid_t uid = 0;

	// check if we're using sudo or setuid
	if (getgid() != getegid() || getuid() != geteuid()) {
		// setuid
		gid = getgid();
		uid = getuid();
	} else {
		// possibly sudo?
		if (getenv("SUDO_GID") == NULL || getenv("SUDO_UID") == NULL)
			FAIL("do not run this program as root directly; use sudo or setuid");

		gid = strtoul(getenv("SUDO_GID"), NULL, 10);
		uid = strtoul(getenv("SUDO_UID"), NULL, 10);
	}

	if (setgid(gid) != 0)
		FAIL_PERROR("cannot drop group privileges");

	if (setuid(uid) != 0)
		FAIL_PERROR("cannot drop user privileges");

	if (chdir("/") != 0)
		FAIL_PERROR("chdir(\"/\")");

	if (getgid() == 0 || getuid() == 0)
		FAIL("cannot drop root privileges");

failure:
	return rc;
}

static int find_first_device(char *output) {
	int rc = 0;

#ifndef DO_NOT_SEARCH
	DIR *acpi_dir;
	struct dirent *entry;

	acpi_dir = opendir(IDEAPAD_ACPI_PATH);
	if (acpi_dir == NULL)
		FAIL_PERROR("opendir()");

	while ((entry = readdir(acpi_dir)) != NULL) {
		if (strncmp(entry->d_name, IDEAPAD_DEVICE_PREFIX, IDEAPAD_DEVICE_PREFIX_LEN) == 0) {
			SNPRINTF_PATH(output, PATH_MAX, "%s/%s", IDEAPAD_ACPI_PATH, entry->d_name);
			return 0;
		}
	}

	FAILf("cannot find device ID in %s", IDEAPAD_ACPI_PATH);
#else
	(void) output;
	FAIL("searching device ID is disabled");
#endif

failure:
	return rc;
}


static void enable_cb(struct tray_menu *menu)
{
	UNUSED int rc;
	int acpi_fd = *(int*) menu->context;
	lseek(acpi_fd, 0, SEEK_SET);
	if (write(acpi_fd, (char[]){'1'}, 1) != 1)
		FAIL_PERROR("cannot write conservation mode");
	return;
failure:
	tray_exit();
}

static void disable_cb(struct tray_menu *menu)
{
	UNUSED int rc;
	int acpi_fd = *(int*) menu->context;
	lseek(acpi_fd, 0, SEEK_SET);
	if (write(acpi_fd, (char[]){'0'}, 1) != 1)
		FAIL_PERROR("cannot write conservation mode");
	return;
failure:
	tray_exit();
}

static void quit_cb(UNUSED struct tray_menu *menu)
{
	tray_exit();
}


static int valid_device_id(char *id) {
	if (strlen(id) != 7)
		return 0;
	if (!strchr(VALID_ID_CHARS, id[0])
		|| !strchr(VALID_ID_CHARS, id[1])
		|| !strchr(VALID_ID_CHARS, id[2])
		|| !strchr(VALID_ID_CHARS, id[3]))
		return 0;
	if (id[4] != ':')
		return 0;
	if (!strchr(VALID_ID_CHARS, id[5])
		|| !strchr(VALID_ID_CHARS, id[6]))
		return 0;

	return 1;
}

int main(int argc, char **argv) {
	int rc;
	int mode_fd = -1;
	char device_path[PATH_MAX];
	char mode_path[PATH_MAX];
	char current_mode = 85; // 1010101
	char last_mode = ~current_mode; // 0101010

	struct tray tray = {
		.icon = DEFAULT_ICON,
		// dont modify the order
		.menu = (struct tray_menu[]) {
			{ .text = "Battery conservation" },
			{ .text = "Enabled",  .checkbox = 1, .cb = enable_cb,  .context = &mode_fd },
			{ .text = "Disabled", .checkbox = 1, .cb = disable_cb, .context = &mode_fd },
			{ .text = "-" },
			{ .text = "Quit", .cb = quit_cb },
			{ .text = NULL }
		}
	};

	// dont modify me
	struct tray_menu *menu_enabled = &tray.menu[1];
	struct tray_menu *menu_disabled = &tray.menu[2];

	// high privilege code

	if (argc > 1) {
		// get the device id from the user
		if (!valid_device_id(argv[1]))
			FAIL("invalid device ID");

		SNPRINTF_PATH(device_path, PATH_MAX, "%s/%s%s", IDEAPAD_ACPI_PATH, IDEAPAD_DEVICE_PREFIX, argv[1]);
	} else {
		if (find_first_device(device_path) != 0)
			goto failure;
	}

	SNPRINTF_PATH(mode_path, PATH_MAX, "%s/conservation_mode", device_path);

	// hopefully we have enough perms
	mode_fd = open(mode_path, O_RDWR);
	if (mode_fd == -1)
		FAIL_PERROR("open()");

	// now we can drop privileges
	if (drop_privilege() != 0)
		goto failure;

	// low privilege code

	if (tray_init(&tray) != 0)
		FAIL("cannot create tray icon");

	while (tray_loop(0) != -1) {
		lseek(mode_fd, 0, SEEK_SET);
		if (read(mode_fd, &current_mode, 1) != 1)
			FAIL_PERROR("cannot read current conservation mode");

		if (current_mode != last_mode) {
			switch (current_mode) {
				case '0':
				menu_enabled->checked = 0;
				menu_disabled->checked = 1;
				tray.icon = CONSERVATION_DISABLED_ICON;
				break;

				case '1':
				menu_enabled->checked = 1;
				menu_disabled->checked = 0;
				tray.icon = CONSERVATION_ENABLED_ICON;
				break;
			}

			tray_update(&tray);
			last_mode = current_mode;
		}
		nanosleep(&WAIT_TIMEOUT, NULL);
	}

failure:
	if (mode_fd != -1)
		close(mode_fd);
	return rc;
}