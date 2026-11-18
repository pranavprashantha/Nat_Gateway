#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <deque>
#include <vector>

class NatTable {
public:
    NatTable(int portStart, int portEnd, long long releaseTime) : releaseTime_(releaseTime) {
        for(int i = portStart; i <= portEnd; i++) {
            ports.push_back(i);
        }
    }

    std::optional<int> translateOutgoing(const std::string& privateIp, int privatePort, long long nowMS) {
        // 1. Build the key from ip and port
        // 2. If the key is already in the outgoing map, return its port
        // 3. Otherwise take nextPort_, add 1 to it, and save the
        //    mapping in BOTH maps
        // 4. Return the new port

        std::string temp = privateIp + ":" + std::to_string(privatePort);
        auto it = outgoing_table.find(temp);

        if(it == outgoing_table.end()) {

            if(ports.empty()) {
                releaseOld(nowMS);
                if(ports.empty()) {
                    return std::nullopt;
                }
            }

            PrivateInfo newInfo = {privateIp, privatePort, nowMS};

            outgoing_table.try_emplace(temp, ports.front());
            incoming_table.try_emplace(ports.front(), newInfo);
            ports.pop_front();
        } else {
            incoming_table[it->second].lastUsed = nowMS;   
        }
        return outgoing_table[temp];
    }

    std::optional<std::pair<std::string, int>> translateIncoming(int publicPort, long long nowMS) {
        // 1. Look up publicPort in the incoming map
        // 2. If it's not there, return std::nullopt
        // 3. Otherwise return the ip and port

        auto it = incoming_table.find(publicPort);

        if(it == incoming_table.end() || nowMS - it->second.lastUsed >= releaseTime_) {
            release(publicPort);
            return std::nullopt;
        }

        it->second.lastUsed = nowMS; 
        PrivateInfo privateinfo = incoming_table[publicPort];
        return std::make_pair(privateinfo.ip, privateinfo.port);
    }

    void release(int publicPort) {
        // 1. Find publicPort in the incoming map. If it's not there, do nothing
        // 2. Use the ip and port stored there to rebuild the outgoing key
        // 3. Erase from BOTH maps
        // 4. Put the port back in freePorts_

        auto it = incoming_table.find(publicPort);

        if(it != incoming_table.end()) {
            auto privateIP = it->second;

            incoming_table.erase(it);
            std::string combinedPrivate = privateIP.ip + ":" + std::to_string(privateIP.port);
            outgoing_table.erase(combinedPrivate);

            ports.push_back(publicPort);
        }
    }

    void releaseOld(long long nowMS) {
        std::vector<int> removeFromTable;
        for(auto& it : incoming_table) {
            if(nowMS - it.second.lastUsed >= releaseTime_) {
                std::string tempPrivate = it.second.ip + ":" + std::to_string(it.second.port);
                int publicPort = outgoing_table[tempPrivate];
                outgoing_table.erase(tempPrivate);
                removeFromTable.push_back(it.first);
                ports.push_back(publicPort);
            }
        }

        for(int i : removeFromTable) {
            incoming_table.erase(i);
        }


    }

private:
    
    // declare your two maps here
    struct PrivateInfo {
        std::string ip;
        int port;
        long long lastUsed;
    };

    std::deque<int> ports;
    std::unordered_map<std::string, int> outgoing_table;
    std::unordered_map<int, PrivateInfo> incoming_table;
    long long releaseTime_;
};