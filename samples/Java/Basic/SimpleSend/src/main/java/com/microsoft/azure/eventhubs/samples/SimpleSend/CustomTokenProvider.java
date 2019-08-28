package com.microsoft.azure.eventhubs.samples.SimpleSend;

static class CustomTokenProvider implements ITokenProvider {
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
                    .thenApply((authResult) -> { try {
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

