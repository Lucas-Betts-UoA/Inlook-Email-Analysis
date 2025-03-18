const SchemaForm = defineComponent({
    name: "schema-form",
    props: {
        schema: Object,
        modelValue: Object,
        nodeCreateFunc: String
    },
    setup(props) {
        const formState = reactive({ ...props.modelValue });
        const pluginRegistryCache = reactive({});

        // Fetch registry values if needed
        const fetchPluginRegistry = async (pluginType) => {
            const apiUrl = `/api/v1/fetch-plugin-registry-implements/${props.nodeCreateFunc}`;
            if (!pluginRegistryCache[pluginType]) {
                try {
                    const response = await fetch(apiUrl);
                    if (!response.ok) throw new Error("Failed to fetch plugin registry");
                    pluginRegistryCache[pluginType] = await response.json(); // Store result
                } catch (error) {
                    console.error(`Error fetching registry for ${pluginType}:`, error);
                    pluginRegistryCache[pluginType] = []; // Avoid re-fetching on error
                }
            }
        };

        const renderField = (key, fieldSchema, value) => {
            if (typeof fieldSchema === "object" && fieldSchema._inlook_check && fieldSchema._inlook_check._PluginRegistry) {
                const pluginType = fieldSchema._inlook_check._PluginRegistry;

                if (!pluginRegistryCache[pluginType]) {
                    fetchPluginRegistry(pluginType);
                }

                return Vue.h("div", { class: "row mb-2 align-items-center" }, [
                    Vue.h("label", { class: "col-sm-3 col-form-label text-end fw-bold" }, key),
                    Vue.h("div", { class: "col-sm-9" }, [
                        Vue.h("select", {
                                class: "form-select form-select-sm",
                            },
                            (pluginRegistryCache[pluginType] || []).map(option =>
                                Vue.h("option", { value: option, selected: option === value }, option)
                            ))
                    ])
                ]);
            }

            if (fieldSchema.type === "string") {
                return Vue.h("div", { class: "row mb-2 align-items-center" }, [
                    Vue.h("label", { class: "col-sm-3 col-form-label text-end fw-bold" }, key),
                    Vue.h("div", { class: "col-sm-9" }, [
                        Vue.h("input", {
                            type: "text",
                            class: "form-control form-control-sm",
                            value: value,
                        })
                    ])
                ]);
            }

            if (fieldSchema.type === "array" && fieldSchema.items) {
                return Vue.h("div", { class: "mb-2" }, [
                    Vue.h("label", { class: "fw-bold" }, key),
                    Vue.h("div", { class: "border rounded p-2" },
                        value.map((item, index) =>
                            Vue.h("div", { class: "row mb-1" }, [
                                Vue.h("label", { class: "col-sm-3 col-form-label text-end small text-muted" }, `Item ${index + 1}`),
                                Vue.h("div", { class: "col-sm-9" }, [
                                    renderField("name", fieldSchema.items.properties.name, item.name),
                                    fieldSchema.items.properties.options
                                        ? renderField("options", fieldSchema.items.properties.options, item.options || {})
                                        : null
                                ])
                            ])
                        )
                    )
                ]);
            }

            if (fieldSchema.type === "object" && fieldSchema.properties) {
                return Vue.h("div", { class: "border rounded p-2 mb-2" }, [
                    Vue.h("label", { class: "fw-bold" }, key),
                    ...Object.entries(fieldSchema.properties).map(([subKey, subSchema]) =>
                        renderField(subKey, subSchema, value ? value[subKey] : "")
                    )
                ]);
            }

            return Vue.h("div");
        };

        return () => Vue.h("form", { class: "bg-white p-3 rounded shadow-sm" }, [
            ...Object.entries(props.schema.properties).map(([key, fieldSchema]) =>
                renderField(key, fieldSchema, formState[key])
            ),
            Vue.h("div", { class: "text-end mt-3" }, [
                Vue.h("button", { class: "btn btn-primary btn-sm", disabled: true }, "Submit (Disabled)")
            ])
        ]);
    }
});
const TreeNodeConfigForm = {
    name: 'TreeNodeConfigForm',
    props: {
        node: {
            type: Object,
            required: true
        }
    },
    components: {
        'schema-form': SchemaForm
    },
    data() {
        return {
            expanded: false
        };
    },
    methods: {
        toggleConfig() {
            this.expanded = !this.expanded;
        }
    },
    template: `
          <ul class="list-unstyled">
            <li>
              <div class="d-flex align-items-center justify-content-between p-2 border rounded">
                <div>
                  <span class="fw-bold">{{ node.instanceID || "Root" }}</span>
                  <span class="ms-2 text-muted">[{{ node.createFunc }}]</span>
                  <span class="badge ms-2"
                        :class="{
                      'bg-primary': node.state === 'LOADED',
                      'bg-success': node.state === 'READY',
                      'bg-warning': node.state === 'RUNNING',
                      'bg-danger': node.state === 'FAILED',
                      'bg-info': node.state === 'COMPLETE',
                      'bg-secondary': node.state === 'UNLOADED'
                    }">
                ({{ node.state }})
              </span>
                </div>

                <button @click="toggleConfig" class="btn btn-sm btn-outline-primary">
                  {{ expanded ? "Hide Config" : "Configure" }}
                </button>
              </div>

              <div v-if="expanded" class="container mt-3 p-3 border rounded shadow-sm bg-white">
                <schema-form v-if="node.schema && node.schema.properties && node.config"
                             :schema="node.schema"
                             :modelValue="node.config"
                             :nodeCreateFunc="node.createFunc">
                </schema-form>
              </div>

              <ul v-if="node.children && node.children.length" class="ms-3">
                <tree-node-config-form v-for="(child, index) in node.children"
                                       :key="child.instanceID || index"
                                       :node="child">
                </tree-node-config-form>
              </ul>
            </li>
          </ul>
        `
};