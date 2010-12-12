#include <getopt.h>
#include "getopt1.h"
#include "client.h"

void initOptions(void)
{
	int opt, longOptInd;
	struct option long_options[] = {
		{"help", 0, 0, 'h'},
		{"host", 1, 0, 'a'},
		{"port", 1, 0, 'p'},
		{"dlevel", 1, 0, 'l'},
		{0, 0, 0, 0}
	};
	int num;

	while (1)
	{
		opt = getopt_long(cfg.argc, cfg.argv,
			"dl:hvp:a:", long_options, &longOptInd);

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
				cli.portstr = optarg;
				break;

			case 'a':
				cli.hoststr = optarg;
				break;

			case 'v':
				cfg.version = 1;
				break;

			case 'l':
				num = atoi(optarg);
				if (num < 0 || num > 9)
					cfg.debugLevel = DFL_DBG_LVL;
				else
					cfg.debugLevel = num;
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

	if (cli.portstr == NULL)
	{
		error("%s: port is unset, aborted.\n", cfg.argv[0]);
		printUsage();
		exit(EXIT_FAILURE);
	}

	if (cli.hoststr == NULL)
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
