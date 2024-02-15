#include "Emitter.h"
#include <algorithm>

void CEmitter::On(const std::string& sEvent, const Callback& callback) 
{
    m_callbacks[sEvent].push_back({ callback, false });
}

void CEmitter::Once(const std::string& sEvent, const Callback& callback) 
{
    m_callbacks[sEvent].push_back({ callback, true });
}

void CEmitter::Emit(const std::string& sEvent, const std::string& sMessage) 
{
    auto it = m_callbacks.find(sEvent);
    if (it != m_callbacks.end()) 
    {
        auto& callbacks = it->second;
        for (auto callbackIt = callbacks.begin(); callbackIt != callbacks.end();) 
        {
            const auto& callbackInfo = *callbackIt;
            callbackInfo.callback(sMessage);

            if (callbackInfo.bIsOnce) {
                callbackIt = callbacks.erase(callbackIt);
            }
            else 
            {
                ++callbackIt;
            }
        }
    }
}

void CEmitter::RemoveCallback(const std::string& sEvent, const Callback& callback) 
{
    auto it = m_callbacks.find(sEvent);
    if (it != m_callbacks.end()) 
    {
        auto& callbacks = it->second;
        callbacks.erase(
            std::remove_if(callbacks.begin(), callbacks.end(),
                [&callback](const CallbackInfo& info) {
                    return info.callback.target<void(const std::string&, const std::string&)>() == callback.target<void(const std::string&, const std::string&)>();
                }),
            callbacks.end()
        );
    }
}