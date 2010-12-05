#include <getopt.h>
#include "getopt1.h"

void initOptions(void)
{
	int opt, longOptInd;
	struct option long_options[] = {
		{"help", 0, 0, 'h'},
		{"port", 1, 0, 'p'},
		{"syslog", 0, 0, 0},
		{"key", 1, 0, 0},
		{0, 0, 0, 0}
	};

	while (1)
	{
		opt = getopt_long(cfg.argc, cfg.argv,
			"dhvp:s", long_options, &longOptInd);

		if (opt == -1)
			break;

		switch (opt)
		{
			case 0:
				switch (longOptInd)
				{
					case 2: /*  --syslog  */
						cfg.syslog = 1;
						break;
					case 3: /* --key  */
						cfg.passkey = optarg;
				}
				break;

			case 'd':
				cfg.debug = 1;
				break;

			case 'h':
				cfg.help = 1;
				break;

			case 's':
				cfg.daemon = 1;
				break;

			case 'p':
				cfg.port = atoi(optarg);
				if (cfg.port == 0)
					cfg.port = -1;
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

	/* When used information argument which leads just echoing
	 * we don't need in strong testing */
	if (cfg.help || cfg.version)
		return;

	/* If we will daemonize server we need to echoing in syslog */
	if (cfg.daemon && !cfg.syslog)
		cfg.syslog = 1;

	if (cfg.port <= 0)
	{
		if (cfg.port < 0)
			error("%s: port is wrong, aborted.\n", cfg.argv[0]);
		else
			error("%s: port is unset, aborted.\n", cfg.argv[0]);
		printUsage();
		exit(EXIT_FAILURE);
	}

	if (cfg.passkey == NULL)
	{
		error("%s: passkey doesn't set manually so passkey is set on default value (see help to know it)\n", cfg.argv[0]);
		cfg.passkey = DEFAULT_PASSKEY;
	}
}

