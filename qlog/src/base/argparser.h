#ifndef __BASE_ARGPARSER__
#define __BASE_ARGPARSER__

#include "base/types.h"
#include "base/string.h"
#include "base/array.h"
#include "base/dictionary.h"

class ArgumentList
{
  Dictionary<String> arguments;
  Array<String> freeArguments;
public:
  ArgumentList()
  {}
  ~ArgumentList()
  {}

  void clear()
  {
    arguments.clear();
    freeArguments.clear();
  }

  void addArgument(String name, String value)
  {
    arguments.set(name, value);
  }
  void addFreeArgument(String value)
  {
    freeArguments.push(value);
  }

  bool hasArgument(String name) const
  {
    return arguments.has(name);
  }
  String getArgumentValue(String name) const
  {
    return arguments.get(name);
  }
  int getFreeArgumentCount() const
  {
    return freeArguments.length();
  }
  String getFreeArgument(int index) const
  {
    return freeArguments[index];
  }
};

class ArgumentParser
{
  struct DefaultValue
  {
    String name;
    String value;
  };
  Array<DefaultValue> defaultValues;
  Dictionary<int> arguments;
  String argumentAbbreviationMap[256];
public:
  ArgumentParser();
  ~ArgumentParser()
  {}

  void registerRequiredArgument(String name, char abbreviation, String defaultValue);
  void registerOptionalArgument(String name, char abbreviation);

  bool parse(int argc, char** argv, ArgumentList& result);
};

#endif // __BASE_ARGPARSER__
