if (!app._context.components["logs"]) {
    app.component("logs", {
        template: '#logs',
        data() {
            return {
                logs: [],
                logSearch: "",
                sortKey: "timestamp",
                sortAsc: true,
                refreshInterval: null, // Stores setInterval reference
            };
        },
        computed: {
            filteredLogs() {
                return this.logs
                    .filter(log =>
                        log.message.toLowerCase().includes(this.logSearch.toLowerCase()) ||
                        log.level.toLowerCase().includes(this.logSearch.toLowerCase())
                    )
                    .sort((a, b) => {
                        if (this.sortAsc) {
                            return a[this.sortKey] > b[this.sortKey] ? 1 : -1;
                        } else {
                            return a[this.sortKey] < b[this.sortKey] ? 1 : -1;
                        }
                    });
            }
        },
        methods: {
            refreshData() {
                this.fetchLogs();
            },
            fetchLogs() {
                this.$root.fetchWithTimeout('/api/v1/fetch-logs')
                    .then(response => response.json())
                    .then(data => {
                        this.logs.push(...data);
                    })
                    .catch(error => this.$root.handleApiError(error));
            },
            initLogs() {
                this.$root.fetchWithTimeout('/api/v1/fetch-init-logs')
                    .then(response => response.json())
                    .then(data => {
                        this.logs = data;
                    })
                    .then(() => this.refreshData())
                    .catch(error => this.$root.handleApiError(error));
            },
            sortBy(key) {
                if (this.sortKey === key) {
                    this.sortAsc = !this.sortAsc;
                } else {
                    this.sortKey = key;
                    this.sortAsc = true;
                }
            }
        },
        mounted() {
            this.initLogs();
            this.fetchLogs(); // Initial log fetch
            this.refreshInterval = setInterval(this.refreshData, 2000); // Refresh logs every 2 seconds
            this.$nextTick(() => {
                console.log("Component fully loaded:", this.$options.name);
            });
        },
        unmounted() {
            clearInterval(this.refreshInterval); // Cleanup interval on component destroy
        }
    });
}