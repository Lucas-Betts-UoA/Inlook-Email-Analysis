if (!app._context.components["plugins"]) {
    app.component("plugins", {
        components: {
            'tree-node-config-form': TreeNodeConfigForm,
            'schema-form': SchemaForm
        },
        template: '#plugins',
        data() {
            return {
                instanceTree: {},
                workflows: [],
                selectedWorkflow: "",
                createFuncs: [],
                refreshInterval: null, // Stores setInterval reference
            };
        },
        methods: {
            refreshData() {
                this.fetchCreateFuncs();
                this.fetchPluginInstances();
                this.fetchWorkflows();
                this.fetchInstanceTree();
            },
            fetchPluginInstances() {
                this.$root.fetchWithTimeout('/api/v1/fetch-plugin-instanceIDs')
                    .then(response => response.json())
                    .then(data => this.instanceTree = data)
                    .catch(error => this.$root.handleApiError(error));
            },
            fetchCreateFuncs() {
                this.$root.fetchWithTimeout('/api/v1/fetch-plugin-name')
                    .then(response => response.json())
                    .then(data => this.createFuncs = data)
                    .catch(error => this.$root.handleApiError(error));
            },
            fetchInstanceTree() {
                this.$root.fetchWithTimeout('/api/v1/fetch-plugin-instance-tree')
                    .then(response => response.json())
                    .then(data => {
                        this.instanceTree = data;
                    })
                    .catch(error => this.$root.handleApiError(error));
            },
            instantiateAllRecursiveFromRoot() {
                this.$root.fetchWithTimeout('/api/v1/instantiate-root-recursive', { method: 'POST' })
                    .then(() => this.refreshData())
                    .catch(error => this.$root.handleApiError(error));
            },
            executeAllRecursiveFromRoot() {
                this.$root.fetchWithTimeout('/api/v1/execute-root-recursive', { method: 'POST' })
                    .then(() => this.refreshData())
                    .catch(error => this.$root.handleApiError(error));
            },
            saveWorkflowConfig() {
                this.$root.fetchWithTimeout('/api/v1/set-workflow-config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(this.workflowConfig)
                })
                    .then(response => {
                        if (!response.ok) {
                            throw new Error("Failed to save workflow config.");
                        }
                        return response.text();
                    })
                    .then(msg => {
                        alert("Workflow configuration saved successfully!");
                    })
                    .catch(error => console.error("Error saving workflow config:", error));
            },
            resetInstances() {
                this.$root.fetchWithTimeout('/api/v1/reset-all-instances', { method: 'POST' })
                    .then(() => this.refreshData())
                    .catch(error => this.$root.handleApiError(error));
            },
            fetchWorkflows() {
                this.$root.fetchWithTimeout('/api/v1/fetch-workflows')
                    .then(response => response.json())
                    .then(data => {
                        this.workflows = data;
                    })
                    .catch(error => this.$root.handleApiError(error));
            },
            setWorkflow() {
                if (!this.selectedWorkflow) {
                    alert("Please select a workflow first.");
                    return;
                }
                this.$root.fetchWithTimeout('/api/v1/set-workflow', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ workflow: this.selectedWorkflow })
                })
                    .then(response => {
                        if (!response.ok) {
                            throw new Error("Failed to set workflow.");
                        }
                    })
                    .catch(error => this.$root.handleApiError(error));
            },
            shutdown() {
                this.$root.fetchWithTimeout('/api/v1/shutdown', { method: 'POST' })
                    .catch(error => this.$root.handleApiError(error));
            },
        },
        mounted() {
            this.refreshData(); // Initial data fetch
            this.refreshInterval = setInterval(this.refreshData, 5000); // Refresh every 5 seconds
            this.$nextTick(() => {
                console.log("Component fully loaded:", this.$options.name);
                let tooltipTriggerList = [].slice.call(document.querySelectorAll('[data-bs-toggle="tooltip"]'));
                tooltipTriggerList.map(tooltipTriggerEl => new bootstrap.Tooltip(tooltipTriggerEl));
            });
        },
        unmounted() {
            clearInterval(this.refreshInterval); // Cleanup interval on component destroy
        }
    });
}