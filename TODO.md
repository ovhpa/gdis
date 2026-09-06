# GDIS Qt6 Development TODO

## Model Editing Dialog — Incomplete Tabs

### Labelling Tab
- Force field labelling (FF labels, elements, distances, counts)
- Import force field definitions
- **Status:** Stubbed UI only, not functional. Currently disabled (`setTabEnabled(false)`).

### Library Tab
- Molecular library save/load
- **Status:** Stubbed UI only, not functional. Currently disabled (`setTabEnabled(false)`).

---

## Notes
- Both tabs were temporarily disabled to allow focus on core functionality.
- Re-enable with `m_tabs->setTabEnabled(index, true)` once implemented.
