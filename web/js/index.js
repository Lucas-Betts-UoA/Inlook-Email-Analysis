const app = Vue.createApp({
    data() {
        return {
            activeTab: 'plugins',
            heartbeatCount: 0,
            heartbeatActive: false,
            heartbeatCountdown: 10,
            heartbeatInterval: null,
            activeTabContent: '',
        };
    },
    methods: {
        setTab(tab) {
            this.activeTab = tab;
            window.location.hash = tab;

            fetch(`tabs/${tab}.html`)
                .then(response => {
                    if (!response.ok) throw new Error(`Failed to fetch tabs/${tab}.html`);
                    return response.text();
                })
                .then(html => {
                    // Create a container for the fetched HTML
                    const container = document.createElement("div");
                    container.innerHTML = html;

                    // If there is a <template>, remove any existing one and add the new one
                    const template = container.querySelector("template");
                    if (template) {
                        const existingTemplate = document.getElementById(tab);
                        if (existingTemplate) {
                            existingTemplate.remove();
                        }
                        document.body.appendChild(template);
                    }

                    // Remove any previously loaded tab scripts
                    document.querySelectorAll(".tab-script").forEach(script => script.remove());

                    // Return a promise that resolves when the new script is loaded
                    return new Promise((resolve, reject) => {
                        const scriptElement = document.createElement("script");
                        scriptElement.src = `/js/${tab}.js`;
                        scriptElement.className = "tab-script";
                        scriptElement.onload = () => {
                            console.log(`Loaded template and script for ${tab}`);
                            resolve();
                        };
                        scriptElement.onerror = () => reject(new Error(`Failed to load script /js/${tab}.js`));
                        document.body.appendChild(scriptElement);
                    });
                })
                .then(() => {
                    // Force Vue to update so that the newly registered component shows up.
                    this.$nextTick(() => {
                        this.$forceUpdate();
                        console.log(`Forced re-render after loading ${tab}`);
                    });
                })
                .catch(error => {
                    console.error(error);
                });
        },
        handleApiError(error) {
            console.error(error);
            this.heartbeatCount++;
            if (this.heartbeatCount >= 5 && !this.heartbeatActive) {
                this.startHeartbeat();
            }
        },
        fetchWithTimeout(url, options = {}, timeout = 5000) {
            return Promise.race([
                fetch(url, options),
                new Promise((_, reject) =>
                    setTimeout(() => reject(new Error("Request timed out")), timeout)
                )
            ]);
        },
        startHeartbeat() {
            this.heartbeatActive = true;
            this.heartbeatCountdown = 10;
            if (this.heartbeatInterval) clearInterval(this.heartbeatInterval);

            this.heartbeatInterval = setInterval(() => {
                fetch('/api/v1/heartbeat')
                    .then(response => {
                        if (response.ok) {
                            this.heartbeatCount = 0;
                            this.heartbeatActive = false;
                            clearInterval(this.heartbeatInterval);
                            this.heartbeatInterval = null;
                        } else {
                            this.heartbeatCountdown = Math.max(this.heartbeatCountdown - 1, 0);
                        }
                    })
                    .catch(err => {
                        console.error("Heartbeat error:", err);
                        this.heartbeatCountdown = Math.max(this.heartbeatCountdown - 1, 0);
                    });
            }, 1000);
        },
        setupWebSocket() {
            const protocol = window.location.protocol === "https:" ? "wss" : "ws";
            const wsUrl = `${protocol}://${window.location.host}/ws`;
            this.ws = new WebSocket(wsUrl);

            this.ws.onopen = () => console.log("WebSocket connected");
            this.ws.onclose = () => console.log("WebSocket disconnected");
            this.ws.onmessage = (event) => this.handleWebSocketMessage(event);
        },
        handleWebSocketMessage(event) {
            try {
                const message = JSON.parse(event.data);
                console.log("WebSocket Message:", message);
                switch (message.event) {
                    case "new_email":
                        this.$emit("fetch-emails");
                        break;
                    case "log_update":
                        this.$emit("fetch-logs");
                        break;
                    case "workflow_update":
                        this.$emit("fetch-workflow");
                        break;
                    default:
                        console.warn("Unhandled WebSocket event:", message.event);
                }
            } catch (error) {
                console.error("Error parsing WebSocket message:", error);
            }
        },
    },

    mounted() {
        const tabFromHash = window.location.hash.replace("#", "");
        if (tabFromHash) {
            this.setTab(tabFromHash);
        } else {
            this.setTab("plugins");
        }
        this.setTab(this.activeTab);
        this.setupWebSocket();
    }
});

app.mount("#app");