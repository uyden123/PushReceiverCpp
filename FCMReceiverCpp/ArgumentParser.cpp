#include "ArgumentParser.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <set>

const wchar_t* argOptionTakesValueString = L"<value> "; //size of 8
const wchar_t* helpAndVersionOptionsSectionOpeningString = L"\nGetting help:\n";
const wchar_t* normalOptionSectionOpeningString = L"\nOptions:\n";
const wchar_t minusSign = '-', equalSign = '=';
const int shortCommandStartPos = 1, longCommandStartPos = 2;

/* ------ DArgumentOption ------ */
CArgumentOption::CArgumentOption() : type(ArgumentOptionType::NormalOption) {}

CArgumentOption::CArgumentOption(std::set<wchar_t>&& _shortCommands, std::set<std::wstring>&& _longCommands, std::wstring _description) : type(ArgumentOptionType::NormalOption), shortCommands(_shortCommands), longCommands(_longCommands), description(std::move(_description)) {}

CArgumentOption::CArgumentOption(ArgumentOptionType _type, std::set<wchar_t>&& _shortCommands, std::set<std::wstring>&& _longCommands, std::wstring _description) : type(_type), shortCommands(_shortCommands), longCommands(_longCommands), description(std::move(_description)) {}

CArgumentOption::CArgumentOption(ArgumentOptionType _type, std::wstring _description) : type(_type), description(std::move(_description)) {}

bool CArgumentOption::AddShortCommand(wchar_t shortCommand) {
    if (shortCommand < 33 || shortCommand == minusSign || shortCommand == 127)
        return false;
    return shortCommands.insert(shortCommand).second;
}

bool CArgumentOption::AddShortCommand(std::set<wchar_t>&& _shortCommands) {
    for (auto shortCommand : _shortCommands)
        if (shortCommand < 33 || shortCommand == minusSign || shortCommand == 127)
            return false;
    shortCommands.insert(_shortCommands.begin(), _shortCommands.end());
    return true;
}

const std::set<wchar_t>& CArgumentOption::ShortCommands() const {
    return shortCommands;
}

void CArgumentOption::ClearShortCommands() {
    shortCommands.clear();
}

bool CArgumentOption::AddLongCommand(const std::wstring& longCommand) {
    if (longCommand.front() == minusSign || longCommand.find(equalSign) != std::string::npos)
        return false;
    return longCommands.insert(longCommand).second;
}

bool CArgumentOption::AddLongCommand(std::set<std::wstring>&& _longCommands) {
    for (auto longCommand : _longCommands)
        if (longCommand.front() == minusSign || longCommand.find(equalSign) != std::string::npos)
            return false;
    longCommands.insert(_longCommands.begin(), _longCommands.end());
    return true;
}

const std::set<std::wstring>& CArgumentOption::LongCommands() const {
    return longCommands;
}

void CArgumentOption::ClearLongCommands() {
    longCommands.clear();
}

void CArgumentOption::AddDescription(const std::wstring& _description) {
    description = _description;
}

ArgumentOptionType CArgumentOption::GetType() const {
    return type;
}

void CArgumentOption::SetType(ArgumentOptionType _type) {
    type = _type;
}

int CArgumentOption::WasSet() const {
    return wasSet;
}

const std::wstring& CArgumentOption::GetValue() const {
    return value;
}

CArgumentParser::CArgumentParser(int argc, wchar_t** argv, std::wstring _appName, std::wstring _appVersion, std::wstring _appDescription) : argumentCount(argc), argumentValues(argv), executableName(getExecutableName(argv[0])), appName(std::move(_appName)), appVersion(std::move(_appVersion)), appDescription(std::move(_appDescription)) {}

std::wstring CArgumentParser::getExecutableName(wchar_t* execCall) {
    std::wstring execName(execCall);
    return execName.substr(execName.find_last_of('/') + 1);
}

std::vector<int> CArgumentParser::calculateSizeOfOptionStrings(const std::vector<CArgumentOption*>& args) {
    std::vector<int> sizes(args.size());
    int index = 0;
    for (auto arg : args) {
        /** formula explanation:
         * 2 * arg->shortCommands.size() -> adding the minus sign to a characters always leads to 2 characters, so we just need to multiply the amount of chars by 2
         * arg->shortCommands.size() + arg->longCommands.size() -> the amount of spaces needed between commands, not reducing by 1 means we'll have one extra space total, but we can account for that when building the string and avoid branches.
         * (arg->type == DArgumentOptionType::InputOption) * 8) -> if arg.type takes a value, we'll print "<value> " (8 characters) after it, because false/true resolves to 0/1, we can safely multiply by this comparison to avoid branching.
         */
        sizes[index] = (int)((2 * arg->shortCommands.size()) + arg->shortCommands.size() + arg->longCommands.size() + ((arg->type == ArgumentOptionType::InputOption) * 8));//strlen("<value> ") == 8
        auto iterator = arg->longCommands.begin(), end = arg->longCommands.end();
        while (iterator != end) {
            /** formula explanation:
             * 2 + (*iterator).size() -> size of string plus two minus signs characters
             */
            sizes[index] += (int)(2 + (*iterator).size());
            ++iterator;
        }
        index++;
    }
    return sizes;
}

std::vector<std::wstring> CArgumentParser::generateOptionStrings(const std::vector<CArgumentOption*>& args, const std::vector<int>& sizes, int columnSize) {
    std::vector<std::wstring> optionStrings;
    optionStrings.reserve(args.size());
    int index = 0;
    for (auto arg : args) {
        std::wostringstream ostringstream;
        ostringstream << "   " << std::setw(columnSize) << std::left;
        std::wstring tempStr;
        tempStr.reserve(sizes[index]);
        for (auto c : arg->shortCommands) {
            tempStr += minusSign;
            tempStr += c;
            tempStr += ' ';
        }
        for (const auto& str : arg->longCommands) {
            tempStr += L"--";
            tempStr += str;
            tempStr += ' ';
        }
        if (arg->type == ArgumentOptionType::InputOption)
            tempStr += argOptionTakesValueString;
        ostringstream << tempStr;
        if (!arg->description.empty())
            ostringstream << "  " << arg->description;
        ostringstream << '\n';
        optionStrings.emplace_back(std::move(ostringstream.str()));
        index++;
    }
    return optionStrings;
}

std::wstring CArgumentParser::generateOptionsSubSection(const std::vector<CArgumentOption*>& args, const wchar_t* openingString) {
    std::wstring sectionString = openingString;
    std::vector<int> otherSizes = calculateSizeOfOptionStrings(args);
    int optionCommandsColSize = 0;
    for (int i = 0; i < args.size(); i++) {
        if (otherSizes[i] > optionCommandsColSize)
            optionCommandsColSize = otherSizes[i];
    }
    auto orderedOptionStrings = generateOptionStrings(args, otherSizes, optionCommandsColSize);
    std::sort(orderedOptionStrings.begin(), orderedOptionStrings.end());
    size_t totalSize = sectionString.size();
    for (const auto& str : orderedOptionStrings)
        totalSize += str.size();
    sectionString.reserve(totalSize);
    for (const auto& str : orderedOptionStrings)
        sectionString += str;
    return sectionString;
}

bool CArgumentParser::isLongCommand(const std::wstring& argument) {
    return (argument.size() > longCommandStartPos && argument[0] == minusSign && argument[1] == minusSign);
}

bool CArgumentParser::isShortCommand(const std::wstring& argument) {
    return (argument.size() > shortCommandStartPos && argument[0] == minusSign && argument[1] != minusSign);
}

bool CArgumentParser::checkIfArgumentIsUnique(CArgumentOption* dArgumentOption) {
    if (argumentOptions.find(dArgumentOption) != argumentOptions.end())
        return false;
    if (dArgumentOption->shortCommands.empty() && dArgumentOption->longCommands.empty())
        return false;
    for (auto argument : argumentOptions) {
        auto shortEnd = argument->shortCommands.end();
        for (auto shortCommand : dArgumentOption->shortCommands)
            if (argument->shortCommands.find(shortCommand) != shortEnd)
                return false;
        auto longEnd = argument->longCommands.end();
        for (auto& longCommand : dArgumentOption->longCommands)
            if (argument->longCommands.find(longCommand) != longEnd)
                return false;
    }
    return true;
}

bool CArgumentParser::checkIfAllArgumentsInListAreUnique(const std::unordered_set<CArgumentOption*>& _arguments) {
    for (auto upperIterator = _arguments.begin(), upperEnd = _arguments.end(); upperIterator != upperEnd; ++upperIterator) {
        if (!checkIfArgumentIsUnique(*upperIterator))
            return false;
        auto lowerIterator = upperIterator;
        if (++lowerIterator == upperEnd)
            break;
        for (; lowerIterator != upperEnd; ++lowerIterator) {
            auto shortEnd = (*lowerIterator)->shortCommands.end();
            for (auto shortCommand : (*upperIterator)->shortCommands)
                if ((*lowerIterator)->shortCommands.find(shortCommand) != shortEnd)
                    return false;
            auto longEnd = (*lowerIterator)->longCommands.end();
            for (auto& longCommand : (*upperIterator)->longCommands)
                if ((*lowerIterator)->longCommands.find(longCommand) != longEnd)
                    return false;
        }
    }
    return true;
}

std::wstring CArgumentParser::generateUsageSection() {
    std::wstring usageString = L"Usage: ";
    std::wstring optionString = argumentOptions.empty() ? std::wstring() : std::wstring(L" [options]");
    std::wstring posArgString;
    if (!positionalArgs.empty()) {
        std::wstring::size_type totalSize = 0;
        for (const auto& posArg : positionalArgs)
            totalSize += std::get<2>(posArg).empty() ? std::get<0>(posArg).size() + 3 : std::get<2>(posArg).size() + 1;
        posArgString.reserve(totalSize);
        for (const auto& posArg : positionalArgs) {
            if (std::get<2>(posArg).empty()) {
                posArgString += L" [";
                posArgString += std::get<0>(posArg);
                posArgString += ']';
                continue;
            }
            posArgString += ' ';
            posArgString += std::get<2>(posArg);
        }
    }
    usageString.reserve(usageString.size() + executableName.size() + optionString.size() + posArgString.size() + 1);
    usageString += executableName;
    usageString += optionString;
    usageString += posArgString;
    usageString += '\n';
    return usageString;
}

std::wstring CArgumentParser::generateDescriptionSection() {
    if (appDescription.empty())
        return {};
    std::wstring descriptionString;
    descriptionString.reserve(2 + appDescription.size());
    descriptionString += '\n';
    descriptionString += appDescription;
    descriptionString += '\n';
    return descriptionString;
}

void CArgumentParser::calculateSizeOfArgumentsString(std::vector<int>& sizes) {
    int index = 0;
    for (auto arg : positionalArgs)
        /** formula explanation:
         * std::get<0>(arg).size() + 2 -> size of string plus size of 2 characters
         */
        sizes[index++] = (int)(std::get<2>(arg).empty() ? (std::get<0>(arg).size() + 2) : std::get<2>(arg).size());
}

std::wstring CArgumentParser::generatePositionalArgsSection() {
    if (positionalArgs.empty())
        return {};
    std::wstring argSection = L"\nArguments:\n";
    std::vector<int> sizes(positionalArgs.size());
    calculateSizeOfArgumentsString(sizes);
    int optionArgsColSize = 0;
    for (int i = 0; i < positionalArgs.size(); i++) {
        if (sizes[i] > optionArgsColSize)
            optionArgsColSize = sizes[i];
    }
    for (auto arg : positionalArgs) {
        std::wostringstream ostringstream;
        ostringstream << "   " << std::setw(optionArgsColSize) << std::left;
        bool customSyntax = !std::get<2>(arg).empty();
        if (customSyntax)
            ostringstream << std::get<2>(arg);
        else {
            std::wstring tempStr;
            tempStr.reserve(2 + std::get<0>(arg).size());
            tempStr += '[';
            tempStr += std::get<0>(arg);
            tempStr += ']';
            ostringstream << tempStr;
        }
        ostringstream << "   " << std::get<1>(arg) << '\n';
        argSection += ostringstream.str();
    }
    return argSection;
}

std::wstring CArgumentParser::generateOptionsSection() {
    std::vector<CArgumentOption*> helpAndVersionOptions, normalOptions;
    for (auto arg : argumentOptions)
        if (arg->type == ArgumentOptionType::HelpOption || arg->type == ArgumentOptionType::VersionOption)
            helpAndVersionOptions.push_back(arg);
        else
            normalOptions.push_back(arg);
    std::wstring sectionString;
    if (!helpAndVersionOptions.empty())
        sectionString += generateOptionsSubSection(helpAndVersionOptions, helpAndVersionOptionsSectionOpeningString);
    if (!normalOptions.empty())
        sectionString += generateOptionsSubSection(normalOptions, normalOptionSectionOpeningString);
    return sectionString;
}

void CArgumentParser::generateErrorText(ParseResult error, const std::wstring& command) {
    switch (error) {
    case ParseResult::InvalidOption:
        errorText = L"Option --" + command + L" is invalid";
        break;
    case ParseResult::ValuePassedToOptionThatDoesNotTakeValue:
        errorText = L"Option --" + command + L" received a value but it doesn't take any";
        break;
    case ParseResult::NoValueWasPassedToOption:
        errorText = L"Option --" + command + L" takes a value but none was passed.";
        break;
    default:
        return;
    }
}

void CArgumentParser::generateErrorText(ParseResult error, wchar_t command) {
    std::wstring str;
    str = command;
    switch (error) {
    case ParseResult::InvalidOption:
        errorText = L"Option -" + str + L" is invalid";
        break;
    case ParseResult::ValuePassedToOptionThatDoesNotTakeValue:
        errorText = L"Option -" + str + L" received a value but it doesn't take any";
        break;
    case ParseResult::NoValueWasPassedToOption:
        errorText = L"Option -" + str + L" takes a value but none was passed.";
        break;
    case ParseResult::OptionsThatTakesValueNeedsToBeSetSeparately:
        errorText = L"Options that takes a value needs to be set separately. Error with option: -" + str;
        break;
    default:
        return;
    }
}

void CArgumentParser::resetParsedValues() {
    positionalArgsValues.clear();
    errorText.clear();
    for (auto arg : argumentOptions) {
        arg->wasSet = 0;
        arg->value.clear();
    }
}

ParseResult CArgumentParser::parseLongCommand(const std::wstring& argument, int& currentIndex) {
    size_t posOfEqualSign = argument.find_first_of(equalSign, longCommandStartPos);
    std::wstring command = argument.substr(longCommandStartPos, posOfEqualSign - longCommandStartPos);
    bool commandFound = false;
    for (auto arg : argumentOptions) {
        auto iterator = arg->longCommands.find(command);
        if (iterator == arg->longCommands.end())
            continue;
        commandFound = true;
        if (arg->type != ArgumentOptionType::InputOption && posOfEqualSign != std::string::npos) {
            generateErrorText(ParseResult::ValuePassedToOptionThatDoesNotTakeValue, command);
            return ParseResult::ValuePassedToOptionThatDoesNotTakeValue;
        }
        if (arg->type == ArgumentOptionType::InputOption) {
            if (posOfEqualSign == std::string::npos) {
                if (++currentIndex == argumentCount) {
                    generateErrorText(ParseResult::NoValueWasPassedToOption, command);
                    return ParseResult::NoValueWasPassedToOption;
                }
                arg->value = std::wstring(argumentValues[currentIndex]);
                if (isLongCommand(arg->value) || isShortCommand(arg->value)) {
                    generateErrorText(ParseResult::NoValueWasPassedToOption, command);
                    return ParseResult::NoValueWasPassedToOption;
                }
            }
            else {
                if (argument[posOfEqualSign] == argument[argument.size() - 1]) {
                    generateErrorText(ParseResult::NoValueWasPassedToOption, command);
                    return ParseResult::NoValueWasPassedToOption;
                }
                arg->value = argument.substr(posOfEqualSign + 1);
            }
        }
        arg->wasSet++;
        break;
    }
    if (!commandFound) {
        generateErrorText(ParseResult::InvalidOption, command);
        return ParseResult::InvalidOption;
    }
    return ParseResult::ParseSuccessful;
}

ParseResult CArgumentParser::parseShortCommand(const std::wstring& argument, int& currentIndex) {
    for (int i = 1; i < argument.size(); i++) {
        bool commandFound = false;
        for (auto arg : argumentOptions) {
            auto iterator = arg->shortCommands.find(argument[i]);
            if (iterator == arg->shortCommands.end())
                continue;
            commandFound = true;
            if (arg->type == ArgumentOptionType::InputOption && argument.size() > 2) {
                generateErrorText(ParseResult::OptionsThatTakesValueNeedsToBeSetSeparately, argument[i]);
                return ParseResult::OptionsThatTakesValueNeedsToBeSetSeparately;
            }
            if (arg->type == ArgumentOptionType::InputOption) {
                if (++currentIndex == argumentCount) {
                    generateErrorText(ParseResult::NoValueWasPassedToOption, argument[i]);
                    return ParseResult::NoValueWasPassedToOption;
                }
                arg->value = std::wstring(argumentValues[currentIndex]);
                if (isLongCommand(arg->value) || isShortCommand(arg->value)) {
                    generateErrorText(ParseResult::NoValueWasPassedToOption, argument[i]);
                    return ParseResult::NoValueWasPassedToOption;
                }
            }
            arg->wasSet++;
            break;
        }
        if (!commandFound) {
            generateErrorText(ParseResult::InvalidOption, argument[i]);
            return ParseResult::InvalidOption;
        }
    }
    return ParseResult::ParseSuccessful;
}

void CArgumentParser::SetAppInfo(const std::wstring& name, const std::wstring& version, const std::wstring& description) {
    appName = name;
    appVersion = version;
    appDescription = description;
}

void CArgumentParser::SetAppName(const std::wstring& name) {
    appName = name;
}

void CArgumentParser::SetAppVersion(const std::wstring& version) {
    appVersion = version;
}

void CArgumentParser::SetAppDescription(const std::wstring& description) {
    appDescription = description;
}

bool CArgumentParser::AddArgumentOption(CArgumentOption* dArgumentOption) {
    if (!checkIfArgumentIsUnique(dArgumentOption))
        return false;
    return argumentOptions.insert(dArgumentOption).second;
}

bool CArgumentParser::AddArgumentOption(std::unordered_set<CArgumentOption*>&& args) {
    if (!checkIfAllArgumentsInListAreUnique(args))
        return false;
    argumentOptions.insert(args.begin(), args.end());
    return true;
}

bool CArgumentParser::RemoveArgumentOption(CArgumentOption* argument) {
    return argumentOptions.erase(argument);
}

void CArgumentParser::ClearArgumentOptions() {
    argumentOptions.clear();
}

void CArgumentParser::AddPositionalArgument(std::wstring name, std::wstring description, std::wstring syntax) {
    positionalArgs.emplace_back(std::move(name), std::move(description), std::move(syntax));
}

void CArgumentParser::ClearPositionalArgumets() {
    positionalArgs.clear();
}

int CArgumentParser::WasSet(char command) {
    for (auto argument : argumentOptions)
        if (argument->shortCommands.find(command) != argument->shortCommands.end())
            return argument->wasSet;
    return 0;
}

int CArgumentParser::WasSet(const std::wstring& command) {
    for (auto argument : argumentOptions)
        if (argument->longCommands.find(command) != argument->longCommands.end())
            return argument->wasSet;
    return 0;
}

const std::vector<std::wstring>& CArgumentParser::GetPositionalArguments() const {
    return positionalArgsValues;
}

std::wstring CArgumentParser::VersionText() {
    std::wstring versionText;
    versionText.reserve(appName.size() + appVersion.size() + 2);
    versionText.append(appName);
    versionText += ' ';
    versionText.append(appVersion);
    versionText += '\n';
    return versionText;
}

std::wstring CArgumentParser::HelpText() {
    std::wstring helpText;
    std::wstring usageSection = generateUsageSection();
    std::wstring descriptionSection = generateDescriptionSection();
    std::wstring posArgsSection = generatePositionalArgsSection();
    std::wstring optionsSection = generateOptionsSection();
    helpText.reserve(usageSection.size() + descriptionSection.size() + posArgsSection.size() + optionsSection.size());
    helpText += usageSection;
    helpText += descriptionSection;
    helpText += posArgsSection;
    helpText += optionsSection;
    return helpText;
}

std::wstring CArgumentParser::ErrorText() const {
    return errorText;
}

ParseResult CArgumentParser::Parse() {
    resetParsedValues();
    if (argumentCount < 2)
        return ParseResult::ParseSuccessful;
    ParseResult parseResult;
    for (int index = 1; index < argumentCount; index++) {
        std::wstring currArg(argumentValues[index]);
        if (isLongCommand(currArg)) {
            parseResult = parseLongCommand(currArg, index);
            if (parseResult != ParseResult::ParseSuccessful)
                return parseResult;
            continue;
        }
        if (isShortCommand(currArg)) {
            parseResult = parseShortCommand(currArg, index);
            if (parseResult != ParseResult::ParseSuccessful)
                return parseResult;
            continue;
        }
        positionalArgsValues.push_back(currArg);
    }
    return ParseResult::ParseSuccessful;
}