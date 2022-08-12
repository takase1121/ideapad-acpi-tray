#define _XOPEN_SOURCE 600

#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tray.h"

#define UNUSED __attribute__((unused))

#define DEFAULT_ICON "face-suprise"
#define CONSERVATION_ENABLED_ICON "face-monkey"
#define CONSERVATION_DISABLED_ICON "face-tired"

const char IDEAPAD_ACPI_PATH[] = "/sys/bus/platform/drivers/ideapad_acpi";
const char IDEAPAD_ACPI_PREFIX[] = "VPC";
const int IDEAPAD_ACPI_PREFIX_LEN = (sizeof(IDEAPAD_ACPI_PREFIX)/sizeof(*IDEAPAD_ACPI_PREFIX)-1);

#define FAILURE(EC, ...) \
	do { \
		fprintf(stderr, "error: " __VA_ARGS__); \
		rc = (EC); \
		goto failure; \
	} while (0)

static int drop_privilege(void) {
	int rc = 0;

	if (setgid(getgid()) != 0)
		FAILURE(2, "cannot drop group privileges: %s\n", strerror(errno));

	if (setuid(getuid()) != 0)
		FAILURE(2, "cannot drop user privileges: %s\n", strerror(errno));

	if (chdir("/") != 0)
		FAILURE(2, "chdir(\"/\"): %s\n", strerror(errno));

	if (getgid() == 0 || getuid() == 0)
		FAILURE(2, "cannot drop root privileges\n");

failure:
	return rc;
}

static int find_first_vpc(char *output, const char * const name) {
	int rc = -1;
	DIR *acpi_dir;
	struct dirent *entry;

	acpi_dir = opendir(IDEAPAD_ACPI_PATH);
	if (acpi_dir == NULL)
		FAILURE(-1, "opendir(%s): %s", IDEAPAD_ACPI_PATH, strerror(errno));

	while ((entry = readdir(acpi_dir)) != NULL) {
		if (strncmp(entry->d_name, IDEAPAD_ACPI_PREFIX, IDEAPAD_ACPI_PREFIX_LEN) == 0)
			return snprintf(output, PATH_MAX, "%s/%s/%s", IDEAPAD_ACPI_PATH, entry->d_name, name);
	}

	FAILURE(-1, "cannot find VPC in %s\n", IDEAPAD_ACPI_PATH);

failure:
	return rc;
}


static void enable_cb(struct tray_menu *menu)
{
	int acpi_fd = *(int*) menu->context;
	lseek(acpi_fd, 0, SEEK_SET);
	if (write(acpi_fd, (char[]){'1'}, sizeof(char)) != sizeof(char))
	{
		fprintf(stderr, "error: write(): %s", strerror(errno));
	}
}


static void disable_cb(struct tray_menu *menu)
{
	int acpi_fd = *(int*) menu->context;
	lseek(acpi_fd, 0, SEEK_SET);
	if (write(acpi_fd, (char[]){'0'}, sizeof(char)) != sizeof(char))
	{
		fprintf(stderr, "error: write(): %s", strerror(errno));
	}
}

static void quit_cb(UNUSED struct tray_menu *menu)
{
	tray_exit();
}

int main(void) {
	int rc;
	int acpi_fd = -1;
	int acpi_path_len = 0;
	char acpi_path[PATH_MAX];
	char conservation_mode = 30;
	char last_mode = 1;

	struct tray tray = {
		.icon = DEFAULT_ICON,
		.menu = (struct tray_menu[]) {
			{ .text = "Battery conservation" },
			{ .text = "Enabled", .checkbox = 1, .cb = enable_cb, .context = &acpi_fd },
			{ .text = "Disabled", .checkbox = 1, .cb = disable_cb, .context = &acpi_fd },
			{ .text = "-" },
			{ .text = "Quit", .cb = quit_cb },
			{ .text = NULL }
		}
	};

	struct tray_menu *menu_enabled = &tray.menu[1];
	struct tray_menu *menu_disabled = &tray.menu[2];


	if (getuid() == 0)
		FAILURE(1, "do not run this program as root directly; use setuid\n");

	// find the first VPC file or user specified value
	acpi_path_len = find_first_vpc(acpi_path, "conservation_mode");
	if (acpi_path_len == -1)
		goto failure;

	// hopefully we have enough perms
	acpi_fd = open(acpi_path, O_RDWR);
	if (acpi_fd == -1)
		FAILURE(errno, "open(%s): %s\n", acpi_path, strerror(errno));

	// now we can drop privileges!
	if ((rc = drop_privilege()) != 0)
		goto failure;


	if (tray_init(&tray) != 0)
		FAILURE(3, "cannot create tray icon\n");

	while (tray_loop(0) != -1) {
		lseek(acpi_fd, 0, SEEK_SET);
		if (read(acpi_fd, &conservation_mode, sizeof(conservation_mode)) != sizeof(conservation_mode))
			FAILURE(errno, "cannot read conservation mode: %s\n", strerror(errno));

		if (conservation_mode != last_mode)
		{
					switch (conservation_mode) {
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
			last_mode = conservation_mode;
		}
		nanosleep((struct timespec[]) {{0, 10000000L}}, NULL);
	}

failure:
	if (acpi_fd != -1)
		close(acpi_fd);
	return rc;
}