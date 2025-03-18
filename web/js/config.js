if (!app._context.components["config"]) {
    app.component("config", {
        template: '#config',
        data() {
            return {
                configEditing: false,
                workflowEditing: false,
                editableGlobalConfig: "",
                editableCurrentWorkflow: "",
                refreshInterval: null,
            };
        },
        methods: {
            fetchGlobalConfig() {
                this.$root.fetchWithTimeout('/api/v1/fetch-global-config')
                    .then(response => response.json())
                    .then(data => {
                        this.editableGlobalConfig = JSON.stringify(data, null, 2);
                    })
                    .catch(error => this.$root.handleApiError(error));
            },
            fetchCurrentWorkflow() {
                this.$root.fetchWithTimeout('/api/v1/fetch-current-workflow')
                    .then(response => response.json())
                    .then(data => {
                        this.editableCurrentWorkflow = JSON.stringify(data, null, 2);
                    })
                    .catch(error => this.$root.handleApiError(error));
            },
            toggleConfigEditing() {
                if (this.configEditing) {
                    this.fetchGlobalConfig();
                }
                this.configEditing = !this.configEditing;
            },
            toggleWorkflowEditing() {
                if (this.workflowEditing) {
                    this.fetchCurrentWorkflow();
                }
                this.workflowEditing = !this.workflowEditing;
            },
            saveConfig() {
                try {
                    const parsedConfig = JSON.parse(this.editableGlobalConfig);
                    this.$root.fetchWithTimeout('/api/v1/set-global-config', {
                        method: 'POST',
                        headers: {'Content-Type': 'application/json'},
                        body: JSON.stringify(parsedConfig)
                    })
                        .then(() => this.configEditing = false)
                        .catch(error => console.error("Error saving global config:", error));
                } catch (error) {
                    alert("Invalid JSON format!");
                }
            },
            saveWorkflow() {
                try {
                    const parsedWorkflow = JSON.parse(this.editableCurrentWorkflow);
                    this.$root.fetchWithTimeout('/api/v1/set-workflow-config', {
                        method: 'POST',
                        headers: {'Content-Type': 'application/json'},
                        body: JSON.stringify(parsedWorkflow)
                    })
                        .then(() => this.workflowEditing = false)
                        .catch(error => console.error("Error saving workflow config:", error));
                } catch (error) {
                    alert("Invalid JSON format!");
                }
            },
            refreshData() {
                this.fetchGlobalConfig();
                this.fetchCurrentWorkflow();
            }
        },
        mounted() {
            this.refreshData(); // Initial fetch
            this.refreshInterval = setInterval(this.refreshData, 5000); // Refresh every 5 seconds
            this.$nextTick(() => {
                console.log("Component fully loaded:", this.$options.name);
            });
        },
        unmounted() {
            clearInterval(this.refreshInterval); // Cleanup interval on component destroy
        }
    });
}