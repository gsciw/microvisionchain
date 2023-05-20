// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Copyright (c) 2021-2023 The MVC developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MVC_NETMESSAGEMAKER_H
#define MVC_NETMESSAGEMAKER_H

#include "net/net.h"
#include "serialize.h"

#include <vector>

class CNetMsgMaker {
public:
    CNetMsgMaker(int nVersionIn) : nVersion(nVersionIn) {}

    template <typename... Args>
    CSerializedNetMsg Make(int nFlags, CSerializedNetMsg::PayloadType payloadType, std::string sCommand,
                           Args &&... args) const {
        std::vector<uint8_t> data;
        CVectorWriter{SER_NETWORK, nFlags | nVersion, data, 0,
                      std::forward<Args>(args)...};
        return {std::move(sCommand), payloadType, std::move(data)};
    }

    template <typename... Args>
    CSerializedNetMsg Make(std::string sCommand, Args &&... args) const {
        return Make(0, CSerializedNetMsg::PayloadType::UNKNOWN, std::move(sCommand), std::forward<Args>(args)...);
    }

    template <typename... Args>
    CSerializedNetMsg Make(CSerializedNetMsg::PayloadType payloadType, std::string sCommand,
                           Args &&... args) const {
        return Make(0, payloadType, std::move(sCommand), std::forward<Args>(args)...);
    }

private:
    const int nVersion;
};

#endif // MVC_NETMESSAGEMAKER_H
