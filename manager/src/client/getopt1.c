#include <getopt.h>
#include "getopt1.h"

void initOptions(void)
{
	int opt, longOptInd;
	struct option long_options[] = {
		{"help", 0, 0, 'h'},
		{"host", 1, 0, 'a'},
		{"port", 1, 0, 'p'},
		{0, 0, 0, 0}
	};

	while (1)
	{
		opt = getopt_long(cfg.argc, cfg.argv,
			"dhvp:a:", long_options, &longOptInd);

		if (opt == -1)
			break;

		switch (opt)
		{
			case 'd':
				cfg.debug = 1;
				break;

			case 'h':
				cfg.help = 1;
				break;

			case 'p':
				cfg.port = optarg;
				break;

			case 'a':
				cfg.host = optarg;
				break;

			case 'v':
				cfg.version = 1;
				break;

			default:
				printUsage();
				exit(EXIT_FAILURE);
		}
	}

	if (optind < cfg.argc)
	{
		error("%s: too many args\n", cfg.argv[0]);
		printUsage();
		exit(EXIT_FAILURE);
	}

	if (cfg.port == NULL)
	{
		error("%s: port is unset, aborted.\n", cfg.argv[0]);
		printUsage();
		exit(EXIT_FAILURE);
	}

	if (cfg.host == NULL)
	{
		error("%s: host is unset, aborted.\n", cfg.argv[0]);
		printUsage();
		exit(EXIT_FAILURE);
	}

	/* When used information argument which leads just echoing
	 * we don't need in strong testing */
	if (cfg.help || cfg.version)
		return;
}
