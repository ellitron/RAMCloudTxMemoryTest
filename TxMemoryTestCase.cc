/* Copyright (c) 2009-2015 Stanford University
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>

#include <iostream>
#include <fstream>

#include "ClusterMetrics.h"
#include "Cycles.h"
#include "Dispatch.h"
#include "ShortMacros.h"
#include "Crc32C.h"
#include "ObjectFinder.h"
#include "OptionParser.h"
#include "RamCloud.h"
#include "Tub.h"
#include "IndexLookup.h"
#include "Transaction.h"

using namespace RAMCloud;

int
main(int argc, char *argv[])
try
{
    int clientIndex;
    int numClients;
    string tableName;
    int serverSpan;
    uint64_t txCount;
    int size;

    // Set line buffering for stdout so that printf's and log messages
    // interleave properly.
    setvbuf(stdout, NULL, _IOLBF, 1024);

    OptionsDescription clientOptions("TransactionsTestCase");
    clientOptions.add_options()

        // These first two options are currently ignored. They're here so that
        // this script can be run with cluster.py.
        ("clientIndex",
         ProgramOptions::value<int>(&clientIndex)->
            default_value(0),
         "Index of this client (first client is 0; currently ignored)")
        ("numClients",
         ProgramOptions::value<int>(&numClients)->
            default_value(1),
         "Total number of clients running (currently ignored)")

        ("tableName",
         ProgramOptions::value<string>(&tableName),
         "Name of the table to perform transaction against.")
        ("serverSpan",
         ProgramOptions::value<int>(&serverSpan)->
            default_value(1),
         "Server span for the table.")
        ("txCount",
         ProgramOptions::value<uint64_t>(&txCount),
         "Number of read/write transactions on the object.")
        ("size",
         ProgramOptions::value<int>(&size),
         "Size of object to read/write in the transaction.");
    
    OptionParser optionParser(clientOptions, argc, argv);

    LOG(NOTICE, "Connecting to %s",
        optionParser.options.getCoordinatorLocator().c_str());

    RamCloud client(optionParser.options.getCoordinatorLocator().c_str());
//    RamCloud client(&optionParser.options);

    uint64_t tableId;
    tableId = client.createTable(tableName.c_str(), serverSpan);

    char buf[size];
    
    Buffer key;
    key.append("key", 3);

    // Write initial value.
    Transaction tx(&client);
    tx.write(tableId,
        key.getRange(0, key.size()),
        key.size(),
        buf,
        size);
    tx.commit();

    for(int i = 0; i < txCount; i++ ) {
      printf("Executing transaction %d\n", i);

      Transaction tx(&client);

      Buffer value;
      tx.read(tableId, 
          key.getRange(0, key.size()),
          key.size(),
          &value);

      tx.write(tableId,
          key.getRange(0, key.size()),
          key.size(),
          value.getRange(0, value.size()),
          value.size());
      
      tx.commit();
    }

    client.dropTable(tableName.c_str());

    printf("Test passed.\n");

    return 0;
} catch (RAMCloud::ClientException& e) {
    fprintf(stderr, "RAMCloud exception: %s\n", e.str().c_str());
    return 1;
} catch (RAMCloud::Exception& e) {
    fprintf(stderr, "RAMCloud exception: %s\n", e.str().c_str());
    return 1;
}
