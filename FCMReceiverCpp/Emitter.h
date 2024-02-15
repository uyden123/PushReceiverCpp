#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>

class CEmitter 
{
public:
    using Callback = std::function<void(const std::string&)>;

    /**
     * Registers a callback function to be executed when the specified event occurs.
     * 
     * @param sEvent The name of the event to listen for.
     * @param callback The callback function to be executed when the event occurs.
     */
    void On(const std::string& sEvent, const Callback& callback);


    /**
     * Registers a callback function to be executed once when the specified event occurs.
     * 
     * @param sEvent The name of the event to listen for.
     * @param callback The callback function to be executed when the event occurs.
     */
    void Once(const std::string& sEvent, const Callback& callback);


    /**
     * Emits the specified event with the given message.
     * 
     * @param sEvent The name of the event to emit.
     * @param sMessage The message to be passed along with the event.
     */
    void Emit(const std::string& sEvent, const std::string& sMessage);

private:
    /**
	 * Removes the specified callback function from the specified event.
	 * 
	 * @param sEvent The name of the event to remove the callback from.
	 * @param callback The callback function to be removed.
	 */
    void RemoveCallback(const std::string& sEvent, const Callback& callback);

private:
    struct CallbackInfo 
    {
        Callback callback;
        bool bIsOnce;
    };

    std::unordered_map<std::string, std::vector<CallbackInfo>> m_callbacks;
};
