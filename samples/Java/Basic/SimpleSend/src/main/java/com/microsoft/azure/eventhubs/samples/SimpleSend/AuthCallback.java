package com.microsoft.azure.eventhubs.samples.SimpleSend;

static class AuthCallback implements AzureActiveDirectoryTokenProvider.AuthenticationCallback {
    final private String clientId;
    final private String clientSecret;

    public AuthCallback(final String clientId, final String clientSecret) {
        this.clientId = clientId;
        this.clientSecret = clientSecret;
    }

    @Override
    public CompletableFuture<String> acquireToken(String audience, String authority, Object state) {
        try {
            ConfidentialClientApplication app = ConfidentialClientApplication.builder(this.clientId, new ClientSecret(this.clientSecret))
                    .authority(authority)
                    .build();
            ClientCredentialParameters parameters = ClientCredentialParameters.builder(Collections.singleton(audience + ".default")).build();
            return app.acquireToken(parameters).thenApply((authResult) -> {
                return authResult.accessToken();
            });
        } catch (Exception e) {
            CompletableFuture<String> failed = new CompletableFuture<String>();
            failed.completeExceptionally(e);
            return failed;
        }
    }
}
