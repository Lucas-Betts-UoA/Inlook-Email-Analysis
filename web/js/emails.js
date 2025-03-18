if (!app._context.components["emails"]) {
    app.component("emails", {
        template: '#emails',
        data() {
            return {
                globalEmails: [],
                currentPage: 0,
                emailsPerPage: 50,
                selectedEmail: null,
                totalEmails: 0,
                showMoreHeaders: false,
            };
        },
        computed: {
            totalPages() {
                return Math.ceil(this.totalEmails / this.emailsPerPage) || 1;
            },
            filteredHeaders() {
                if (!this.selectedEmail) return {};
                const importantKeys = ["From", "To", "Date", "Subject", "Content-Type"];
                return Object.fromEntries(
                    Object.entries(this.selectedEmail.headers).filter(([key]) => !importantKeys.includes(key))
                );
            }
        },
        methods: {
            fetchEmailCount() {
                this.$root.fetchWithTimeout('/api/v1/fetch-email-count')
                    .then(response => response.json())
                    .then(data => this.totalEmails = data)
                    .catch(error => this.$root.handleApiError(error));
            },
            fetchEmails(page = this.currentPage) {
                this.currentPage = page;
                const startIdx = page * this.emailsPerPage;

                this.$root.fetchWithTimeout(`/api/v1/fetch-emails/${startIdx}/${this.emailsPerPage}`)
                    .then(response => response.json())
                    .then(data => {
                        data.forEach(email => {
                            if (email.headers["Content-Transfer-Encoding"]?.toLowerCase() === "quoted-printable") {
                                email.body = this.decodeQuotedPrintable(email.body);
                            }
                            email.body = this.escapeHTML(email.body);
                        });
                        this.globalEmails = data;
                    })
                    .catch(error => this.$root.handleApiError(error));
            },
            refreshData() {
                this.fetchEmailCount();
                this.fetchEmails(this.currentPage);
            },
            viewEmail(index) {
                this.selectedEmail = {...this.globalEmails[index]};
            },
            escapeHTML(html) {
                const div = document.createElement("div");
                div.textContent = html;
                return div.innerHTML;
            },
            decodeQuotedPrintable(text) {
                return text
                    .replace(/=\r?\n/g, '')
                    .replace(/=([A-Fa-f0-9]{2})/g, (_, hex) => String.fromCharCode(parseInt(hex, 16)));
            },
            prevPage() {
                if (this.currentPage > 0) this.fetchEmails(this.currentPage - 1);
            },
            nextPage() {
                if (this.globalEmails.length >= this.emailsPerPage) this.fetchEmails(this.currentPage + 1);
            }
        },
        mounted() {
            this.fetchEmailCount();
            this.fetchEmails();
            this.refreshInterval = setInterval(this.refreshData, 1000);
            this.$nextTick(() => {
                console.log("Component fully loaded:", this.$options.name);
            });
        },
        unmounted() {
            clearInterval(this.refreshInterval); // Cleanup interval on component destroy
        }
    });
}