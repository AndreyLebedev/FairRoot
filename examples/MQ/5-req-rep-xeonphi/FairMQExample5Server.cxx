/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample5Server.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExample5Server.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample5Server::FairMQExample5Server()
{
}

void FairMQExample5Server::CustomCleanup(void *data, void *hint)
{
    delete (string*)hint;
}

void FairMQExample5Server::Run()
{
    while (CheckCurrentState(RUNNING))
    {
        unique_ptr<FairMQMessage> request(NewMessage());

        if (Receive(request, "data") >= 0)
        {
            LOG(INFO) << "Received request from client: \"" << string(static_cast<char*>(request->GetData()), request->GetSize()) << "\"";

            // This array will be copied to the card, filled there and copied back
            char replyStr[1024];
            char hostname[100];
            gethostname(hostname,sizeof(hostname));
            // This part of the code will be exwcuted on the Phi
            #pragma offload target(mic)
            {
               char michostname[100];
               gethostname(michostname, sizeof(michostname));
               sprintf(replyStr, "MIC: Hello from MIC. I am %s and I have %ld logical cores. I was called from host: %s \n", michostname, sysconf(_SC_NPROCESSORS_ONLN), hostname);
            }

            // Construct reply string
            string* text = new string("Reply from server: " + string(replyStr));

            LOG(INFO) << "Sending reply to client.";

            unique_ptr<FairMQMessage> reply(NewMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text));

            Send(reply, "data");
        }
    }
}

FairMQExample5Server::~FairMQExample5Server()
{
}
