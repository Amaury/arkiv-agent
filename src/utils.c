/*
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "yvar.h"
#include "yjson.h"
*/

#include "ystr.h"
#include "yarray.h"
#include "yfile.h"
#include "yexec.h"
#include "ybin.h"
#include "yansi.h"
#include "configuration.h"
#include "utils.h"

/* Tells if a given program is installed. */
bool check_program_exists(const char *bin_name) {
	ystr_t ys = get_program_path(bin_name);
	if (!ys)
		return (false);
	ys_free(ys);
	return (true);
}
/* Returns a path to a program. */
ystr_t get_program_path(const char *bin_name) {
	ystr_t path = NULL;

	if (!bin_name)
		return (NULL);
	// check /bin, /usr/bin, /usr/local/bin
	ys_printf(&path, "/bin/%s", bin_name);
	if (yfile_is_executable(path))
		return (path);
	ys_printf(&path, "/usr/bin/%s", bin_name);
	if (yfile_is_executable(path))
		return (path);
	ys_printf(&path, "/usr/local/bin/%s", bin_name);
	if (yfile_is_executable(path))
		return (path);
	ys_delete(&path);
	// use 'which' program
	if (!yfile_is_executable("/usr/bin/which"))
		return (NULL);
	ybin_t data = {0};
	yarray_t args = yarray_create(1);
	yarray_push(&args, (void*)bin_name);
	ystatus_t status = yexec("/usr/bin/which", args, NULL, &data, NULL);
	yarray_free(args);
	if (status == YENOERR) {
		path = ys_copy(data.data);
		ys_trim(path);
	}
	if (status != YENOERR || ys_empty(path))
		ys_delete(&path);
	ybin_delete_data(&data);
	return (path);
}

/* Checks if the tar program is installed. Aborts if not. */
void check_tar(void) {
	if (check_program_exists("tar"))
		return;
	printf(YANSI_RED "Unable to find 'tar' program on this computer." YANSI_RESET "\n\n");
	printf("Please, install " YANSI_GOLD "tar" YANSI_RESET " in a standard location ("
	       YANSI_PURPLE "/bin/tar" YANSI_RESET ", " YANSI_PURPLE "/usr/bin/tar" YANSI_RESET " or "
	       YANSI_PURPLE "/usr/local/bin/tar" YANSI_RESET ") and try again.\n");
	printf(YANSI_FAINT "See " YANSI_LINK " for more information.\n\n" YANSI_RESET, "https://doc.arkiv.sh/agent/install", "the documentation");
	printf(YANSI_RED "Abort" YANSI_RESET "\n");
	exit(2);
}
/* Checks if the sha512sum program is installed. Aborts if not. */
void check_sha512sum(void) {
	if (check_program_exists("sha512sum"))
		return;
	printf(YANSI_RED "Unable to find 'sha512sum' program on this computer." YANSI_RESET "\n\n");
	printf("Please, install " YANSI_GOLD "sha512sum" YANSI_RESET " in a standard location ("
	       YANSI_PURPLE "/bin/sha512sum" YANSI_RESET ", " YANSI_PURPLE "/usr/bin/sha512sum" YANSI_RESET " or "
	       YANSI_PURPLE "/usr/local/bin/sha512sum" YANSI_RESET ") and try again.\n");
	printf(YANSI_FAINT "See " YANSI_LINK " for more information.\n\n" YANSI_RESET, "https://doc.arkiv.sh/agent/install", "the documentation");
	printf(YANSI_RED "Abort" YANSI_RESET "\n");
	exit(2);
}
/* Checks if compression programs are installed. */
void check_z(void) {
	bool hasGzip = (check_program_exists("gzip") && check_program_exists("gunzip")) ? true : false;
	bool hasBzip2 = (check_program_exists("bzip2") && check_program_exists("bunzip2")) ? true : false;
	bool hasXz = (check_program_exists("xz") && check_program_exists("unxz")) ? true : false;
	bool hasZstd = (check_program_exists("zstd") && check_program_exists("unzstd")) ? true : false;
	if (hasGzip && hasBzip2 && hasXz && hasZstd)
		return;
	if (!hasGzip && !hasBzip2 && !hasXz && !hasZstd) {
		printf(YANSI_RED "Unable to find any supported compression program." YANSI_RESET "\n\n");
		printf("You may install " YANSI_GOLD "gzip" YANSI_RESET ", " YANSI_GOLD "bzip2" YANSI_RESET ", "
		       YANSI_GOLD "xz" YANSI_RESET " or " YANSI_GOLD "zstd" YANSI_RESET " in a standard location ("
		       YANSI_PURPLE "/bin" YANSI_RESET ", " YANSI_PURPLE "/usr/bin" YANSI_RESET " or "
		       YANSI_PURPLE "/usr/local/bin" YANSI_RESET ") or proceed without compression.\n");
	} else {
		printf("Here are the compression software installed on this computer:\n");
		printf("%s zstd    " YANSI_RESET, (hasZstd ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
		printf(YANSI_FAINT "(2015) Not much installed; good compression level and very high speed\n" YANSI_RESET);
		printf("%s xz      " YANSI_RESET, (hasXz ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
		printf(YANSI_FAINT "(2009) Not installed everywhere; the best compression ratio, but rather slow\n" YANSI_RESET);
		printf("%s bzip2   " YANSI_RESET, (hasBzip2 ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
		printf(YANSI_FAINT "(1996) Commonly installed; very good compression level and decent speed\n" YANSI_RESET);
		printf("%s gzip    " YANSI_RESET, (hasGzip ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
		printf(YANSI_FAINT "(1992) A standard; decent speed and compression ratio\n" YANSI_RESET);
	}
	printf("\nDo you want to continue? [" YANSI_YELLOW "Y" YANSI_RESET "/" YANSI_YELLOW "n" YANSI_RESET "] " YANSI_BLUE);
	fflush(stdout);
	ystr_t ys = NULL;
	ys_gets(&ys, stdin);
	printf(YANSI_RESET);
	ys_trim(ys);
	bool shouldContinue = (ys_empty(ys) || !strcasecmp(ys, "y") || !strcasecmp(ys, "yes")) ? true : false;
	ys_free(ys);
	printf("\n");
	if (!shouldContinue) {
		printf(YANSI_RED "Abort." YANSI_RESET "\n");
		exit(2);
	}
}
/* Checks if encryption programs are installed. */
void check_crypt(void) {
	bool hasOpenssl = check_program_exists("openssl");
	bool hasScrypt = check_program_exists("scrypt");
	bool hasGpg = check_program_exists("gpg");
	if (hasOpenssl && hasScrypt && hasGpg)
		return;
	if (!hasOpenssl && !hasScrypt && !hasGpg) {
		printf("\n" YANSI_BG_RED " Unable to find any supported encryption program " YANSI_RESET "\n\n");
		printf("You must install " YANSI_GOLD "openssl" YANSI_RESET ", " YANSI_GOLD "scrypt" YANSI_RESET " or "
		       YANSI_GOLD "gpg" YANSI_RESET " in a standard location (" YANSI_PURPLE "/bin" YANSI_RESET ", "
		       YANSI_PURPLE "/usr/bin" YANSI_RESET " or " YANSI_PURPLE "/usr/local/bin" YANSI_RESET ").\n\n");
		printf(YANSI_RED "Abort." YANSI_RESET "\n");
		exit(2);
	}
	printf("Here are the encryption software installed on this computer:\n");
	printf("%s gpg      " YANSI_RESET, (hasGpg ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
	printf(YANSI_FAINT "GNU's implementation of the OpenPGP standard\n" YANSI_RESET);
	printf("%s scrypt   " YANSI_RESET, (hasScrypt ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
	printf(YANSI_FAINT "Very secure; slow by design\n" YANSI_RESET);
	printf("%s openssl  " YANSI_RESET, (hasOpenssl ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
	printf(YANSI_FAINT "Not designed for encrypting large files\n" YANSI_RESET);
	printf("\nDo you want to continue? [" YANSI_YELLOW "Y" YANSI_RESET "/" YANSI_YELLOW "n" YANSI_RESET "] " YANSI_BLUE);
	fflush(stdout);
	ystr_t ys = NULL;
	ys_gets(&ys, stdin);
	printf(YANSI_RESET);
	ys_trim(ys);
	bool shouldContinue = (ys_empty(ys) || !strcasecmp(ys, "y") || !strcasecmp(ys, "yes")) ? true : false;
	ys_free(ys);
	printf("\n");
	if (!shouldContinue) {
		printf(YANSI_RED "Abort." YANSI_RESET "\n");
		exit(2);
	}
}
/* Checks if web communication programs are installed. */
void check_web(void) {
	bool hasWget = check_program_exists("wget");
	bool hasCurl = check_program_exists("curl");
	if (hasWget || hasCurl)
		return;
	printf("\n" YANSI_BG_RED " Unable to find any supported web communication program " YANSI_RESET "\n\n");
	printf("You must install " YANSI_GOLD "wget" YANSI_RESET " or " YANSI_GOLD "curl" YANSI_RESET
	       " in a standard location (" YANSI_PURPLE "/bin" YANSI_RESET ", " YANSI_PURPLE "/usr/bin" YANSI_RESET
	       " or " YANSI_PURPLE "/usr/local/bin" YANSI_RESET ").\n\n");
	printf(YANSI_RED "Abort." YANSI_RESET "\n");
	exit(2);
}
/* Checks if a crontab execution can be planned. */
config_crontab_t check_cron(void) {
	if (yfile_is_dir("/etc/cron.hourly") && yfile_is_writable("/etc/cron.hourly"))
		return (A_CONFIG_CRON_HOURLY);
	if (yfile_is_dir("/etc/cron.d") && yfile_is_writable("/etc/cron.d"))
		return (A_CONFIG_CRON_D);
	if (yfile_exists("/etc/crontab") && yfile_is_writable("/etc/crontab"))
		return (A_CONFIG_CRON_CRONTAB);
	printf("\n" YANSI_BG_RED " Unable to find any writable crontab file " YANSI_RESET "\n\n");
	printf("It should be available under the directories " YANSI_YELLOW "/etc/cron.hourly" YANSI_RESET
	       " or " YANSI_YELLOW "/etc/cron.d" YANSI_RESET ",\nor the file " YANSI_YELLOW "/etc/crontab" YANSI_RESET
	       ".\n\n");
	printf(YANSI_BOLD "Maybe you forgot to execute the agent program as super-user?\n\n" YANSI_RESET);
	printf(YANSI_RED "Abort." YANSI_RESET "\n");
	exit(2);
}
/* Check if database dump programs (mysqldump, pg_dump) are installed. */
void check_database_dump(void) {
	bool hasMysqldump = check_program_exists("mysqldump");
	bool hasPgdump = check_program_exists("pg_dump");
	bool hasMongodump = check_program_exists("mongodump");
	if (hasMysqldump && hasPgdump && hasMongodump)
		return;
	if (!hasMysqldump && !hasPgdump && !hasMongodump) {
		printf(YANSI_RED "Unable to find any database dump program." YANSI_RESET "\n"
		       "You will not be able to back up any MySQL, Postgresql or MongoDB database.\n\n");
		printf("You may install " YANSI_GOLD "mysqldump" YANSI_RESET ", " YANSI_GOLD "pg_dump" YANSI_RESET
		       " or " YANSI_GOLD "mongodump" YANSI_RESET " in a standard location ("
		       YANSI_PURPLE "/bin" YANSI_RESET ", " YANSI_PURPLE "/usr/bin" YANSI_RESET " or "
		       YANSI_PURPLE "/usr/local/bin" YANSI_RESET ") or proceed without database backups.\n");
	} else {
		printf("Here are the database dump software installed on this computer:\n");
		printf("%s mysqldump   " YANSI_RESET, (hasMysqldump ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
		printf(YANSI_FAINT "Default backup software for MySQL databases\n" YANSI_RESET);
		printf("%s pg_dump     " YANSI_RESET, (hasPgdump ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
		printf(YANSI_FAINT "Default backup software for PostgreSQL databases\n" YANSI_RESET);
		printf("%s mongodump   " YANSI_RESET, (hasMongodump ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
		printf(YANSI_FAINT "Default backup software for MongoDB databases\n" YANSI_RESET);
	}
	printf("Do you want to continue? [" YANSI_YELLOW "Y" YANSI_RESET "/" YANSI_YELLOW "n" YANSI_RESET "] " YANSI_BLUE);
	fflush(stdout);
	ystr_t ys = NULL;
	ys_gets(&ys, stdin);
	printf(YANSI_RESET);
	ys_trim(ys);
	bool shouldContinue = (ys_empty(ys) || !strcasecmp(ys, "y") || !strcasecmp(ys, "yes")) ? true : false;
	ys_free(ys);
	printf("\n");
	if (!shouldContinue) {
		printf(YANSI_RED "Abort." YANSI_RESET "\n");
		exit(2);
	}
}

