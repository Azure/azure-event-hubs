/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventhubs.samples.rbac;

import java.text.ParseException;
import java.time.Duration;
import java.util.Collections;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionException;

import com.microsoft.aad.msal4j.*;
import com.microsoft.azure.eventhubs.*;
import com.microsoft.azure.eventhubs.impl.ClientConstants;

class CustomTokenProvider implements ITokenProvider {
    final private String authority;
    final private String audience = ClientConstants.EVENTHUBS_AUDIENCE;
    final private String clientId;
    final private String clientSecret;

    public CustomTokenProvider(final String authority, final String clientId, final String clientSecret) {
        this.authority = authority;
        this.clientId = clientId;
        this.clientSecret = clientSecret;
    }

    @Override
    public CompletableFuture<SecurityToken> getToken(String resource, Duration timeout) {
        try {
            ConfidentialClientApplication app = ConfidentialClientApplication.builder(this.clientId, new ClientSecret(this.clientSecret))
                    .authority(authority)
                    .build();
            ClientCredentialParameters parameters = ClientCredentialParameters.builder(Collections.singleton(audience + ".default")).build();
            return app.acquireToken(parameters)
                    .thenApply((authResult) -> { 
                        try {
                            return new JsonSecurityToken(authResult.accessToken(), resource);
                        } catch (ParseException e) {
                            throw new CompletionException(e);
                        }
                    });
        }
        catch (Exception e) {
            CompletableFuture<SecurityToken> failed = new CompletableFuture<SecurityToken>();
            failed.completeExceptionally(e);
            return failed;
        }
    }
}

