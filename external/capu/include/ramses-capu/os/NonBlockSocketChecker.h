/*
* Copyright (C) 2014 BMW Car IT GmbH
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef RAMSES_CAPU_NONBLOCKSOCKETCHECKER_H
#define RAMSES_CAPU_NONBLOCKSOCKETCHECKER_H

#include "ramses-capu/os/Socket.h"
#include "ramses-capu/util/Delegate.h"
#include "ramses-capu/container/Pair.h"

namespace ramses_capu
{

    /**
     * The NonBlockSocketChecker can check several sockets for incoming data with one call
     * using nonblocking IO
     */
    class NonBlockSocketChecker
    {
    public:
        /**
        * Returns the internal socket descriptor. This can be different on
        * Checks if data is available for read in a vector of sockets. If data is available the
        * callback inside the SocketInfoPair is called. With this it is possible to track
        * multiple connections with one process
        *
        *@param vector with SocketInfoPairs to track
        */
        static status_t CheckSocketsForIncomingData(const vector<ramses_capu::os::SocketInfoPair>& sockets);

        /**
        * Checks if data is available for read in a vector of sockets. If data is available the
        * callback inside the SocketInfoPair is called. With this it is possible to track
        * multiple connections with one process
        *
        * @param vector with SocketInfoPairs to track
        * @param milliseconds to wait until the call returns if no data is available in all of the given sockets
        */
        static status_t CheckSocketsForIncomingData(const vector<ramses_capu::os::SocketInfoPair>& sockets, uint32_t milliseconds);

    private:
        static status_t CheckSocketsForIncomingDataInternal(const vector<ramses_capu::os::SocketInfoPair>& socketsToCheck, timeval* timeout);
    };

    inline status_t NonBlockSocketChecker::CheckSocketsForIncomingData(const vector<ramses_capu::os::SocketInfoPair>& socketsToCheck)
    {
        return CheckSocketsForIncomingDataInternal(socketsToCheck, 0);
    }

    inline status_t NonBlockSocketChecker::CheckSocketsForIncomingData(const vector<ramses_capu::os::SocketInfoPair>& socketsToCheck, uint32_t timeoutMillis)
    {
        timeval timeout;

        timeout.tv_sec = timeoutMillis / 1000;
        timeout.tv_usec = (timeoutMillis % 1000) * 1000;

        return CheckSocketsForIncomingDataInternal(socketsToCheck, &timeout);
    }

    inline status_t NonBlockSocketChecker::CheckSocketsForIncomingDataInternal(const vector<ramses_capu::os::SocketInfoPair>& socketsToCheck, timeval* timeout)
    {
        fd_set fdset;
        FD_ZERO(&fdset);

        vector<ramses_capu::os::SocketInfoPair>::ConstIterator current = socketsToCheck.begin();
        const vector<ramses_capu::os::SocketInfoPair>::ConstIterator end = socketsToCheck.end();

        int_t maxfd = -1;
        for (; current != end; ++current)
        {
            ramses_capu::os::SocketInfoPair socketInfo = *current;
            const ramses_capu::os::SocketDescription& sockedDescription = socketInfo.first;

            if (static_cast<int_t>(sockedDescription) > maxfd)
            {
                maxfd = sockedDescription;
            }
            FD_SET(sockedDescription, &fdset);
        }

        if (maxfd != -1)
        {
            const int_t result = select(static_cast<int32_t>(maxfd + 1), &fdset, NULL, NULL, timeout);


            if (result > 0)
            {
                current = socketsToCheck.begin();

                for (; current != end; ++current)
                {
                    ramses_capu::os::SocketInfoPair socketInfo = *current;
                    const ramses_capu::os::SocketDescription& sockedDescription = socketInfo.first;

                    if (FD_ISSET(sockedDescription, &fdset))
                    {
                        socketInfo.second(sockedDescription);
                    }
                }
            }
        }

        return CAPU_OK;
    }

}

#endif // RAMSES_CAPU_NONBLOCKSOCKETCHECKER_H
