# GDIS Qt6 Development TODO

The following are under review/developement.

## FILE Import/Export

- Open/Save of DIFFaX file format is untested and disabled (a long time ago).
- Import Project is untested.
- Import Geomview is untested.

## Edit menu

- Undo is untested.
- Colour is a stub (replace with some hook to the Model Editing dialog/remove).

## Tools menu

### Visualization submenu

- Periodic table: test if we can conserve/reset some atomic properties (WIP).

### Building submenu

#### Model Editing Dialog — Incomplete Tabs

##### Labelling Tab
- Force field labelling (FF labels, elements, distances, counts)
- Import force field definitions
- **Status:** Stubbed UI only, not functional. Currently disabled (`setTabEnabled(false)`).

Note: Re-enable with `m_tabs->setTabEnabled(index, true)` once implemented.

##### Library Tab
- Molecular library save/load
- **Status:** Stubbed UI only, not functional. Currently disabled (`setTabEnabled(false)`).

Note: Re-enable with `m_tabs->setTabEnabled(index, true)` once implemented.

#### MD Initializer dialog

- Require models to be load in a specific order. A little odd and rigid interface, could use some modern view.

### Computation submenu

- GULP: tested on version 6.0
- GAMESS: tested on version 2022.R2
- Monty: untested
- SIESTA: untested (WIP version 5.4.2)
- VASP: tested on version 5.4.4
- USPEX: untested (WIP version 9.4.4/10.4)

### Analysis submenu

- Dynamics works but there is a graphic bug (WIP).
- Plots Frequency not implemented yet (WIP).

## View Menu

### Display Properties dialog

- Camera animation creation untested.
- Lights: Directional type is added as a Positional no matter what (WIP).
- OpenGL change graphics font is registered in `~/.gdisrc` and remembered but some kind of confirmation would be great.
- Stereo was change to Red/Cyan Red/Blue anaglyph due to the lack of software quad-buffer.

### Reset model image

- missing a refresh/redraw and need\_clear.
- normal/recording mode untested. (I have never used it so I have no idea how it works).

## Help Menu

- The manual is not displayed (WIP, for now the function is a stub).

## Toolbar

- Multiple canvas is not available at the moment (WIP).
- Record mode untested. (I have never used it so I have no idea how it works).
- Graph controls dialog: The data set properties was suspended because of some major bugs and confusion caused by LLM (WIP: rethink the dialog as a whole). NOTE: since markup is not available anymore, the HTML-like marking is still here but ignored.







