#include <stdio.h>
#include "base/argparser.h"
#include "base/file.h"
#include "replay.h"
#include "actiondump.h"

int main(int argc, char** argv)
{
  ArgumentParser argumentParser;
  argumentParser.registerRequiredArgument("detail", 'd', "simple");
  argumentParser.registerRequiredArgument("output", 'o', "log.txt");
  ArgumentList arguments;
  if (!argumentParser.parse(argc, argv, arguments) || arguments.getFreeArgumentCount() != 1)
  {
    printf("DotaReplayManager qlog utility (DRM 3.02d)\n");
    printf("Copyright (C) 2013 d07.RiV. All rights reserved.\n");
    printf("\n");
    printf("Usage:   qlog [options] <file name>\n");
    printf("         <file name> - full or relative path to replay file\n");
    printf("\n");
    printf("Options:\n");
    printf("         -d --detail <detail> - detail mode; simple or full (default: simple)\n");
    printf("         -o --output <file_name> - path to output file (default: \"log.txt\")\n");
    return 1;
  }

  int detail = arguments.getArgumentValue("detail").icompare("simple") ? DUMP_FULL : DUMP_SIMPLE;
  File* output = File::open(arguments.getArgumentValue("output"), File::REWRITE);
  if (output == NULL)
  {
    printf("Failed to open \"%s\" for writing\n", arguments.getArgumentValue("output"));
    return 1;
  }
  uint32 error;
  W3GReplay* w3g = W3GReplay::load(arguments.getFreeArgument(0), &error);
  if (w3g)
  {
    FileActionLogger logger(output);
    ActionDumper dumper(w3g);
    dumper.dump(&logger, detail);
    delete w3g;
  }
  else
  {
    if (error == W3GReplay::eNoFile)
    {
      output->printf("Failed to open \"%s\"\r\n", arguments.getFreeArgument(0));
      printf("Failed to open \"%s\"\n", arguments.getFreeArgument(0));
    }
    else if (error == W3GReplay::eBadFile)
    {
      output->printf("Failed to parse replay\r\n");
      printf("Failed to parse replay\n");
    }
    delete output;
    return 1;
  }

  delete output;
  return 0;
}
