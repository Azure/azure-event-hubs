/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
import com.google.common.collect.Lists;
import com.twitter.hbc.ClientBuilder;
import com.twitter.hbc.core.Client;
import com.twitter.hbc.core.Constants;
import com.twitter.hbc.core.Hosts;
import com.twitter.hbc.core.HttpHosts;
import com.twitter.hbc.core.endpoint.StatusesFilterEndpoint;
import com.twitter.hbc.core.event.Event;
import com.twitter.hbc.core.processor.StringDelimitedProcessor;
import com.twitter.hbc.httpclient.auth.Authentication;
import com.twitter.hbc.httpclient.auth.OAuth1;

import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * This code sample shows how to using Twitter's Streaming API in conjunction with the EventHub
 * producer to populate an EventHub instance with tweets. In this case, we focus on tweets
 * pertaining to the U.S. presidential election just for fun - we use keywords to filter the tweets, so
 * this approach isn't terribly accurate.
 *
 * This example uses the Hosebird Client found here:
 * https://github.com/twitter/hbc
 *
 * Also, using Java 8
 */
public class TwitterProducerSample {
    public static void main( String[] args ) {
        if (args.length < 4) {
            System.err.println("Usage: TwitterProducerSample <consumerKey> <consumerSecret> <token> <tokenSecret>");
            System.exit(1);
        }

        /** Set up your blocking queues: Be sure to size these properly based on expected TPS of your stream */
        BlockingQueue<String> msgQueue = new LinkedBlockingQueue<>(100000);
        BlockingQueue<Event> eventQueue = new LinkedBlockingQueue<>(1000);

        /** Declare the host you want to connect to, the endpoint, and authentication (basic auth or oauth) */
        Hosts hosebirdHosts = new HttpHosts(Constants.STREAM_HOST);
        StatusesFilterEndpoint hosebirdEndpoint = new StatusesFilterEndpoint();

        // Optional: set up some followings and track terms
        List<String> terms = Lists.newArrayList("election", "president", "presidential");
        hosebirdEndpoint.trackTerms(terms);

        // These secrets should be read from a config file
        // Checkout https://dev.twitter.com for more info about this
        Authentication hosebirdAuth = new OAuth1(args[0], args[1], args[2], args[3]);

        ClientBuilder builder = new ClientBuilder()
                .name("Hosebird-Test-Client")
                .hosts(hosebirdHosts)
                .authentication(hosebirdAuth)
                .endpoint(hosebirdEndpoint)
                .processor(new StringDelimitedProcessor(msgQueue))
                .eventMessageQueue(eventQueue);

        Client hosebirdClient = builder.build();
        hosebirdClient.connect();

        // How many threads do you need?
        for (int i = 0; i < 100000000; i++) {
            TweetHandler t = new TweetHandler(msgQueue);
            t.start();
        }
    }
}
