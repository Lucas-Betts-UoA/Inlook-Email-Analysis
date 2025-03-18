if (!app._context.components["readme"]) {
    app.component("readme", {
        template: "#readme",
        data() {
            return {
                markdownContent: "",
            };
        },
        methods: {
            fetchReadme() {
                fetch("/api/v1/readme")
                    .then(response => {
                        if (!response.ok) throw new Error("Failed to fetch README.md");
                        return response.text();
                    })
                    .then(md => {
                        this.markdownContent = marked.parse(md);
                        document.getElementById("readme-content").innerHTML = this.markdownContent;
                    })
                    .catch(error => {
                        console.error(error);
                        document.getElementById("readme-content").innerHTML =
                            "<p class='text-danger'>Failed to load README.md</p>";
                    });
            }
        },
        mounted() {
            this.fetchReadme();
            this.$nextTick(() => {
                console.log("Component fully loaded:", this.$options.name);
            });
        }
    });
}