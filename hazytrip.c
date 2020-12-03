/*
 * hazytrip - tripcode bruteforcer for futaba-type imageboards 
 * copyright (c) 2020 derelictpillows
 */

/* usage:
 * $ hazytrip [OPTION] "SEARCH STRING"
 * 
 * build:
 * gcc -O3 -ansi -march=native -o hazytrip hazytrip.c -fopenmp -lcrypto -Wall -Wextra
 *
 * note:
 * tripcodes can only be 10 chars long
 * tripcodes can only contain the characters from the range ./0-9A-Za-z
 * the 10th character of a tripcode can only be from these characters - .26AEIMQUYcgkosw
 */
  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <openssl/des.h>

#ifdef _OPENMP
#ifdef __APPLE
	#include <libiomp/omp.h>
#else
	#include <omp.h>
#endif
#else
	#define omp_init_lock(mutex) ;
	#define omp_destroy_lock(mutex) ;
	#define omp_set_lock(mutex) ;
	#define omp_unset_lock(mutex) ;
	#define omp_test_lock(mutex) ;
	#define omp_get_num_procs(void) 1
	#define omp_get_thread_num(void) 0
	typedef unsigned char omp_lock_t;
#endif

struct _global {
	const char *name;
	const char *desc;
	const char *version;
	const char *author;
};

const struct _global GLOBAL = {
	.name = "hazytrip",
	.desc = "tripcode bruteforcer for futaba-type imageboards",
	.version = "1.0.0",
	.author = "derelictpillows",
};

enum _program_mode {
	HELP_MSG = -1,
	NO_QUERY_MODE = 0,
	CASE_SENSITIVE = 1,
	CASE_AGNOSTIC = 2
};
typedef enum _program_mode pmode_t;

enum _avg_stats {
	COUNT_ONLY,
	FETCH_DATA
};

#define NUM_OF_ERRORS 4

enum _error {
	ERROR_NO_QUERY,
	ERROR_QUERY_LENGTH,
	ERROR_QUERY_INVALID,
	ERROR_QUERY_TENTH_CHAR
};

struct _error_msg {
	enum _error err;
	const char *msg;
};

const struct _error_msg ERROR_LIST[NUM_OF_ERRORS] = {
	{ .err = ERROR_NO_QUERY, .msg = "Give me a query string next time." },
	{ .err = ERROR_QUERY_LENGTH, .msg = "tripcodes can be no longer than 10 characters" },
	{ .err = ERROR_QUERY_INVALID, .msg = "tripcodes can only have the range ./0-9A-Za-z in them" },
	{ .err = ERROR_QUERY_TENTH_CHAR, .msg = "the 10th character of a tripcode can only be one of these characters, you know! .26AEIMQUYcgkosw" }
};

void print_error(enum _error err)
{
	fprintf(stderr, "[!] Error! -- %s\n", ERROR_LIST[err].msg);
}

void cli_splash(const unsigned num_cores)
{
	fprintf(stdout, "%s %s\n", GLOBAL.name, GLOBAL.version);
	fprintf(stdout, "%s\nReleased.\n", GLOBAL.author);
	fprintf(stdout, "Using %u thread", num_cores);
	if (num_cores > 1)
		fputc('s', stdout);
	fprintf(stdout, ".%c", '\n');
	unsigned i;
	for (i = 0; i < 64; i++)
		fprintf(stdout, "%c", '-');
	fprintf(stdout, "%c", '\n');
	fflush(stdout);
}

void cli_help_msg(void)
{
	fprintf(stdout, "usage:\n\t%s [OPTION] \"SEARCH STRING\"\n", GLOBAL.name);
	fprintf(stdout, "help:\n");
	fprintf(stdout, "\t(None)\t no query. hazytrip will print random tripcodes to stdout.\n");
	fprintf(stdout, "\t-i\t case agnostic search.\n");
	fprintf(stdout, "\t-h\t display this help screen.\n");
	fprintf(stdout, "note:\n");
	unsigned i;
	for (i = 1; i < NUM_OF_ERRORS; i++)
	{
		fprintf(stdout, "\t%s\n", ERROR_LIST[i].msg);
	}
}

int validate_query(const char *query)
{
	static const unsigned QUERY_MAX_LENGTH = 10;
	static const unsigned TENTH_CHAR_CANDIDATES = 16;
	static const char *tenth_char = ".26AEIMQUYcgkosw";
	if (!query)
	{
		print_error(ERROR_NO_QUERY);
		return 0;
	}
	unsigned len = strlen(query);
	if (len > QUERY_MAX_LENGTH)
	{
		print_error(ERROR_QUERY_LENGTH);
		return 0;
	}
	unsigned i;
	for (i = 0; i < len; i++)
	{
		if ( !( ( query[i] >= '.' && query[i] <= '9') ||
					(query[i] >= 'A' && query[i] <= 'Z') ||
					(query[i] >= 'a' && query[i] <= 'z') ) )
		{
			print_error(ERROR_QUERY_INVALID);
			return 0;
		}
	}
	if (len == QUERY_MAX_LENGTH)
	{
		unsigned match_found = 0;
		for (i = 0; i < TENTH_CHAR_CANDIDATES; i++)
		{
			if (query[len - 1] == tenth_char[i])
				match_found = 1;
		}
		if (!match_found)
		{
			print_error(ERROR_QUERY_TENTH_CHAR);
			return 0;
		}
	}
	return 1;
}

static unsigned QRAND_SEED;

void seed_qrand(unsigned seed)
{
	QRAND_SEED = seed;
}

int qrand(void)
{
	QRAND_SEED = (214013 * QRAND_SEED + 2531011);
	return (QRAND_SEED >> 16) & 0x7FFF;
}

void seed_qrand_r(unsigned *seeds, unsigned num)
{
	unsigned i;
	for (i = 0; i < num; i++)
	{
		int random_value = 0;
		while (!random_value)
			random_value = qrand();
		int j = 0;
		while (j++ != random_value)
			qrand();
		seeds[i] = qrand();
	}
}

int qrand_r(unsigned *seed)
{
	*seed = (214013 * *seed + 2531011);
	return (*seed >> 16) & 0x7FFF;
}

unsigned trip_frequency(enum _avg_stats mode)
{
	static unsigned current_tally = 0;
	static unsigned average = 0;
	static time_t time_at_last_call = 0;
	time_t current_time = time(NULL);
	
	if (mode == FETCH_DATA)
		return (average) ? average : current_tally;
	else
	{
		if (current_time != time_at_last_call)
		{
			average = (average / 2.0) + (current_tally / 2.0);
			current_tally = 1;
		}
		else
			current_tally++;
		time_at_last_call = current_time;
	}
	return 0;
}

float trip_rate_condense(const unsigned rate, char *prefix)
{
	#define K_TRIP 1000.0f
	#define MAGS 5
	static const char trip_prefix[MAGS] = {'\0', 'k', 'm', 'g', 't' };
	static const float trip_magnitude[MAGS] = {
		0.0,
		K_TRIP,
		K_TRIP * K_TRIP,
		K_TRIP * K_TRIP * K_TRIP,
		K_TRIP * K_TRIP * K_TRIP * K_TRIP,
	};
	unsigned i;
	for (i = 0; i < MAGS; i++)
	{
		if (rate < trip_magnitude[i] && i != 0)
		{
			*prefix = trip_prefix[i - 1];
			return (float) rate / trip_magnitude[i - 1];
		}
		else
			continue;
	}
	return (float) rate;
}

static const unsigned char PASSWORD_LENGTH = 8;
static const unsigned char SALT_LENGTH = 4;
static const unsigned char DES_FCRYPT_LENGTH = 14;
static const unsigned char TRIPCODE_LENGTH = 10;

void generate_password(char *password, unsigned *seed)
{
	/* 
	 * shift-JIS is a legacy 2 byte encoding, and many *chans convert
	 * the more exotic characters to UTF-8, leading to unpredictable tripcodes
	 */
	static const unsigned char TABLE_SIZE = 92;
	static const char *lookup = " !\"$%&\'()*+,-./0123456789:;<=>?"
	"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_abcdefghijklmnopqrstuvwxyz{|}";
	/*
	 * '#' triggers secure tripcodes on 4chan
	 * '~' and '\' don't have 1 byte Shift-JIS equivalents
	 */
	unsigned char i;
	for (i = 0; i < PASSWORD_LENGTH; i++)
	{
		password[i] = lookup[qrand_r(seed) % TABLE_SIZE];
	}
	password[PASSWORD_LENGTH] = '\0';
}

void generate_salt(const char *password, char *salt)
{
	salt[0] = password[1];
	salt[1] = password[2];
	salt[2] = 'H';
	salt[3] = '.';
	salt[4] = '\0';
}

void strip_outliers(char *salt)
{
	unsigned char i;
	for (i = 0; i < SALT_LENGTH; i++)
	{
		if (salt[i] < '.' || salt[i] > 'z')
			salt[i] = '.';
	}
}

void replace_punctuation(char *salt)
{
	unsigned char i;
	for (i = 0; i < SALT_LENGTH; i++)
	{
		if (salt[i] >= ':' && salt[i] <= '@')
			salt[i] += 0x06;
		else if (salt[i] >= '[' && salt[i] <= '`')
			salt[i] += 0x06;
	}
}

void truncate_tripcode(char *hash)
{
	const static unsigned char HASH_OFFSET = 3;
	memmove(hash, hash+HASH_OFFSET, TRIPCODE_LENGTH);
	hash[TRIPCODE_LENGTH] = '\0';
}

char *strcasestr(const char *haystack, const char *needle)
{
	unsigned len_h = strlen(haystack);
	unsigned len_n = strlen(needle);
	unsigned i, j;
	for (i = 0; i < len_h; i++)
	{
		unsigned matches = 0;
		for (j = 0; j < len_n; j++)
		{
			if (i + len_n <= len_h)
			{
				char h = haystack[i + j];
				char n = needle[j];
				h += (h >= 'A' && h <= 'Z') ? 0x20 : 0x00;
				n += (h >= 'A' && h <= 'Z') ? 0x20 : 0x00;
				if (h == n)
					matches++;
			}
			else
				break;
		}
		if (matches == len_n)
			return (char *) haystack + i;
	}
	return NULL;
}

void determine_match(pmode_t mode, char *query, char *trip, char *password, omp_lock_t *io_lock)
{
	switch (mode)
	{
		case CASE_AGNOSTIC: if (strcasestr(trip, query)) goto print; break;
		case CASE_SENSITIVE: if (strstr(trip, query)) goto print; break;
		case NO_QUERY_MODE: goto print; break; /* extremely slow. */
		default: break;
	}
	return;

	print:
	{
		char prefix = '\0';
		unsigned avg_rate = trip_frequency(FETCH_DATA);
		float avg_float = trip_rate_condense(avg_rate, &prefix);

		omp_set_lock(io_lock);
		fprintf(stdout, "TRIP: '!%s' -> PASS: '%.8s' ", trip, password);
		if (prefix)
			fprintf(stdout, "@ %.2f %cTrip/s\n", avg_rate);
		else
			fprintf(stdout, "@ %u Trip/s\n", avg_rate);
		omp_unset_lock(io_lock);
	}
}

int main(int argc, char **argv)
{
	omp_lock_t io_lock;
	omp_init_lock(&io_lock);
	const unsigned NUM_CORES = omp_get_num_procs();
	cli_splash(NUM_CORES);

	seed_qrand(time(NULL));
	unsigned qrand_seeds[NUM_CORES];
	seed_qrand_r(qrand_seeds, NUM_CORES);

	pmode_t mode;
	if (argc == 1)
		mode = NO_QUERY_MODE;
	else if (!strcmp(argv[1], "-h"))
	{
		cli_help_msg();
		return 1;
	}
	else
	{
		mode = (!strcmp(argv[1], "-i")) ? CASE_AGNOSTIC : CASE_SENSITIVE;
		if (!validate_query(argv[mode]))
			return 1;
	}

#ifdef _OPENMP
	#pragma omp parallel num_threads(NUM_CORES)
#endif
	{
		const unsigned THREAD_ID = omp_get_thread_num();
		while (1)
		{
			/*
			 * Intel Core2 Duo P8600 @ 2.401GHz with 2 threads
			 * Case sensitive - 353.1 kTrips/s
			 * Case agnostic - 347.3 kTrips/s
			 */
			char password[PASSWORD_LENGTH + 1];
			char salt[SALT_LENGTH + 1];
			generate_password(password, &qrand_seeds[THREAD_ID]);
			generate_salt(password, salt);
			strip_outliers(salt);
			replace_punctuation(salt);
			char trip[DES_FCRYPT_LENGTH];
			DES_fcrypt(password, salt, trip);
			truncate_tripcode(trip);
			determine_match(mode, argv[mode], trip, password, &io_lock);
			trip_frequency(COUNT_ONLY);
		}
	}
	omp_destroy_lock(&io_lock);
	return 0;
}
