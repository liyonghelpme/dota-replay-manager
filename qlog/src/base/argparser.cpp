#include "argparser.h"

ArgumentParser::ArgumentParser()
  : arguments(DictionaryMap::asciiNoCase)
{}
void ArgumentParser::registerRequiredArgument(String name, char abbreviation, String defaultValue)
{
  int position = defaultValues.length();
  DefaultValue& val = defaultValues.push();
  val.name = name;
  val.value = defaultValue;
  arguments.set(name, position);
  argumentAbbreviationMap[(uint8) abbreviation] = name;
}
void ArgumentParser::registerOptionalArgument(String name, char abbreviation)
{
  arguments.set(name, -1);
  argumentAbbreviationMap[(uint8) abbreviation] = name;
}

bool ArgumentParser::parse(int argc, char** argv, ArgumentList& result)
{
  result.clear();

  // if no arguments specified - treat as an error
  if (argc <= 1)
    return false;

  // list of specified required arguments; used in the end to apply default values
  Array<bool> requiredArgumentsSet;
  requiredArgumentsSet.resize(defaultValues.length(), false);

  for (int i = 1; i < argc; ++i)
  {
    if (argv[i][0] != '-' || argv[i][1] == 0)
    {
      // argument does not start with dash or equals "-"
      result.addFreeArgument(argv[i]);
    }
    else if (argv[i][1] == '-')
    {
      // full argument name
      int nameEnd = 2;
      while (argv[i][nameEnd] && argv[i][nameEnd] != '=')
        nameEnd++;

      String name(argv[i] + 2, nameEnd - 2);
      if (!arguments.has(name))
        return false; // argument not found
      int argumentId = arguments.get(name);
      if (argumentId >= 0)
      {
        //required argument
        requiredArgumentsSet[argumentId] = true;
        if (argv[i][nameEnd] != '=')
        {
          if (i + 1 < argc)
          {
            // value separated with space
            ++i;
            nameEnd = 0;
          }
          else
          {
            return false;
          }
        }
      }
      if (nameEnd == 0) // value separated with space
        result.addArgument(name, argv[i]);
      else if (argv[i][nameEnd] == '=') // value separated with '='
        result.addArgument(name, argv[i] + nameEnd + 1);
      else // no value
        result.addArgument(name, "");
    }
    else
    {
      // abbreviation
      String name = argumentAbbreviationMap[(uint8) argv[i][1]];
      if (name.isEmpty() || !arguments.has(name))
        return false; // argument not found
      int argumentId = arguments.get(name);
      int valueStart = 2;
      if (argumentId >= 0)
      {
        // required argument
        requiredArgumentsSet[argumentId] = true;
        if (argv[i][2] == 0)
        {
          if (i + 1 < argc)
          {
            // value separated with space
            ++i;
            valueStart = 0;
          }
          else
          {
            return false;
          }
        }
      }
      result.addArgument(name, argv[i] + valueStart);
    }
  }
  for (int i = 0; i < defaultValues.length(); ++i)
  {
    if (!requiredArgumentsSet[i])
    {
      result.addArgument(defaultValues[i].name, defaultValues[i].value);
    }
  }
  return true;
}
