if (!app._context.components["docs"]) {
    app.component("docs", {
        template: '#docs',
        data() {
            return {
                docsPath: "", // Will store the correct iframe URL
            };
        },
        methods: {
            fetchDocsAvailability() {
                fetch('/api/v1/fetch-docs-exist') // Call an API to check if docs exist
                    .then(response => response)
                    .then(data => {
                        this.docsPath = "/docs/index.html"; // Set iframe path
                        console.log(this.docsPath);
                    })
                    .catch(error => {
                        console.error("Error checking docs availability:", error);
                        this.docsPath = "about:blank";
                    });
            }
        },
        mounted() {
            this.fetchDocsAvailability();
            this.$nextTick(() => {
                console.log("Component fully loaded:", this.$options.name);
            });
        }
    });
}