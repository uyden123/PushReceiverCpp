#pragma once
#ifndef DARGUMENTPARSER_LIBRARY_H
#define DARGUMENTPARSER_LIBRARY_H

#include <string>
#include <vector>
#include <set>
#include <unordered_set>

enum class ParseResult : unsigned char {
    ParseSuccessful,
    InvalidOption,
    ValuePassedToOptionThatDoesNotTakeValue,
    NoValueWasPassedToOption,
    OptionsThatTakesValueNeedsToBeSetSeparately
};

enum class ArgumentOptionType : unsigned char {
    NormalOption,
    InputOption,
    HelpOption,
    VersionOption
};

class CArgumentOption {
    friend class CArgumentParser;

    ArgumentOptionType type = ArgumentOptionType::NormalOption;
    int wasSet = 0;
    std::wstring value;
    std::set<wchar_t> shortCommands;
    std::set<std::wstring> longCommands;
    std::wstring description;

public:

    /**
     * @details instantiate a DArgumentOption with default values (type = DArgumentOptionType::NormalOption)
     */
    explicit CArgumentOption();

    CArgumentOption(std::set<wchar_t>&& _shortCommands, std::set<std::wstring>&& _longCommands, std::wstring _description = std::wstring());

    CArgumentOption(ArgumentOptionType _type, std::set<wchar_t>&& _shortCommands, std::set<std::wstring>&& _longCommands, std::wstring _description = std::wstring());

    CArgumentOption(ArgumentOptionType _type, std::wstring _description);

    /**
     * Adds the passed character to the command list for this option, unless it was already included.
     * @return true if command was added, false if command was invalid(1) or not added.
     * @def invalid(1) - any character without a ascii representation, spaces or the character minus(-).
     */
    bool AddShortCommand(wchar_t shortCommand);

    /**
     * Adds the passed characters to the command list for this option, if any of the characters were already added it won't be added again.
     * @return false if any of the commands were invalid(1), otherwise true.
     * @def invalid(1) - any character without a ascii representation, spaces or the character minus(-).
     */
    bool AddShortCommand(std::set<wchar_t>&& _shortCommands);

    [[nodiscard]] const std::set<wchar_t>& ShortCommands() const;

    void ClearShortCommands();

    /**
     * Adds the passed string to the command list for this option, unless it was already included.
     * @return true if command was added, false if command was invalid(1) or not added.
     * @def invalid(1) - if string starts with a minus(-) sign or has a equal(=) sign.
     */
    bool AddLongCommand(const std::wstring& longCommand);

    /**
     * Adds the passed strings to the command list for this option, if any of the strings were already added it won't be added again.
     * @return false if any of the commands were invalid(1), otherwise true.
     * @def invalid(1) - if string starts with a minus(-) sign or has a equal(=) sign.
     */
    bool AddLongCommand(std::set<std::wstring>&& _longCommands);

    [[nodiscard]] const std::set<std::wstring>& LongCommands() const;

    void ClearLongCommands();

    void AddDescription(const std::wstring& _description);

    void SetType(ArgumentOptionType _type);

    [[nodiscard]] ArgumentOptionType GetType() const;

    [[nodiscard]] int WasSet() const;

    [[nodiscard]] const std::wstring& GetValue() const;
};

class CArgumentParser {
    int argumentCount;
    wchar_t** argumentValues;
    const std::wstring executableName;
    std::wstring appName;
    std::wstring appVersion;
    std::wstring appDescription;
    std::unordered_set<CArgumentOption*> argumentOptions;
    std::vector<std::tuple<std::wstring, std::wstring, std::wstring>> positionalArgs;
    std::vector<std::wstring> positionalArgsValues;
    std::wstring errorText;

    static std::wstring getExecutableName(wchar_t* execCall);

    static std::vector<int> calculateSizeOfOptionStrings(const std::vector<CArgumentOption*>& args);

    static std::vector<std::wstring> generateOptionStrings(const std::vector<CArgumentOption*>& args, const std::vector<int>& sizes, int columnSize);

    static std::wstring generateOptionsSubSection(const std::vector<CArgumentOption*>& args, const wchar_t* openingString);

    static bool isLongCommand(const std::wstring& argument);

    static bool isShortCommand(const std::wstring& argument);

    bool checkIfArgumentIsUnique(CArgumentOption* dArgumentOption);

    bool checkIfAllArgumentsInListAreUnique(const std::unordered_set<CArgumentOption*>& _arguments);

    std::wstring generateUsageSection();

    std::wstring generateDescriptionSection();

    void calculateSizeOfArgumentsString(std::vector<int>& sizes);

    std::wstring generatePositionalArgsSection();

    std::wstring generateOptionsSection();

    void generateErrorText(ParseResult error, const std::wstring& command);

    void generateErrorText(ParseResult error, wchar_t command);

    void resetParsedValues();

    ParseResult parseLongCommand(const std::wstring& argument, int& currentIndex);

    ParseResult parseShortCommand(const std::wstring& argument, int& currentIndex);

public:

    CArgumentParser(int argc, wchar_t** argv, std::wstring _appName = std::wstring(), std::wstring _appVersion = std::wstring(), std::wstring _appDescription = std::wstring());

    void SetAppInfo(const std::wstring& name, const std::wstring& version, const std::wstring& description = std::wstring());

    void SetAppName(const std::wstring& name);

    void SetAppVersion(const std::wstring& version);

    void SetAppDescription(const std::wstring& description);

    /**
     * <br>if the argument is valid(1) then it will be added to the argument list.
     * @return true if argument was added, false if it wasn't (invalid argument).
     * @def valid(1) - At least 1 command, either long or short, is set and all of its commands are unique (when compared to other DArgumentOptions added before).
     * @details Do not use in-place constructors as you won't be able to remove it later and clearing all the argumentOptions will result in the memory being leaked.
     */
    bool AddArgumentOption(CArgumentOption* dArgumentOption);

    /**
     * <br>if the argument is valid(1) then it will be added to the argument list.
     * @return true if argument was added, false if it wasn't (invalid argument).
     * @def valid(1) - At least 1 command, either long or short, is set and all of its commands are unique (when compared to other DArgumentOptions added before).
     */
    bool AddArgumentOption(CArgumentOption& dArgumentOption) { return AddArgumentOption(&dArgumentOption); }

    /**
     * <br>if all argumentOptions are valid(1) then they will be added to the argument list.
     * @return true if the argumentOptions were added, false if they were not. (at least one argument was invalid).
     * @def valid(1) - At least 1 command, either long or short, is set for each argument and all commands are unique (when compared to other DArgumentOptions added before and to each other).
     * @details Do not use in-place constructors for DArgumentOption as you won't be able to remove it later and clearing all the argumentOptions will result in the memory being leaked.
     */
    bool AddArgumentOption(std::unordered_set<CArgumentOption*>&& args);

    /**
     * Removes the passed argument from the argument list.
     * @return true if it was removed, false if it wasn't (in case there was no such argument in the list).
     */
    bool RemoveArgumentOption(CArgumentOption* argument);

    /**
     * Removes the passed argument from the argument list.
     * @return true if it was removed, false if it wasn't (in case there was no such argument in the list).
     */
    bool RemoveArgumentOption(CArgumentOption& argument) { return RemoveArgumentOption(&argument); }

    /**
     * Removes all argumentOptions previously added, clearing the list.
     */
    void ClearArgumentOptions();

    /**
     * <br>Used to generate the help string.
     * @param name Name of the command.
     * @param description The description to be shown.
     * @param syntax How the command should be used, useful for more complex applications. (Optional, will default to the passed name if not set).
     */
    void AddPositionalArgument(std::wstring name, std::wstring description, std::wstring syntax = std::wstring());

    /**
     * Removes all positional arguments previously added, clearing the list.
     */
    void ClearPositionalArgumets();

    /**
     * @param command the command character to check.
     * @return Returns a boolean indicating if the option was set or not, always returns false if no option with specified command was found.
     */
    [[nodiscard]] int WasSet(char command);

    /**
     * @param command the command string to check.
     * @return Returns a boolean indicating if the option was set or not, always returns false if no option with specified command was found.
     */
    [[nodiscard]] int WasSet(const std::wstring& command);

    /**
     * Retrieves the value of every positional argument that was set during the parsing.
     * @return Returns a const reference to the positionalArgsValues list.
     */
    [[nodiscard]] const std::vector<std::wstring>& GetPositionalArguments() const;

    [[nodiscard]] std::wstring VersionText();

    [[nodiscard]] std::wstring HelpText();

    [[nodiscard]] std::wstring ErrorText() const;

    /**
     * <br>Parses the argv passed on creation based on the positional arguments and option arguments added.
     * @return true if parse was successful, false if an error occurred (non-optional parameter not passed). Call "ErrorText" function to retrieve a printable string of the error.
     */
    ParseResult Parse();
};

#endif //DARGUMENTPARSER_LIBRARY_H
