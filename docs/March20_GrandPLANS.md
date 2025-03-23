## TimeBuster Grand Plans

March 20, 2025

#### Refactor again?

I've been worried about proper documentation and project planning. This program is getting a bit out of control, and I'm at risk of losing the project if I don't begin more top-down, structured planning and transition fully out of the "vibe coding" pattern I've been using so far.

I've been letting the A.I. do the work of planning *and* coding, and only been rubber-stamping design decisions. I need to be more explicit: keep my own record of all the criticism and needed changes, and check-off their completion myself. 

So what are my actual big-picture stages of development? My own goals?

### Stage 1: Foundation Set
[ ] Implement basic editing for events and todos
[ ] Finish encapsulation of backends, possibly create backend factory. Make `CollectionController` and `ConfigManager` totally neutral.
[ ] Get delta handling implemented.
[ ] Basic unit tests for all foundational functions, integration tests for their coherency
[ ] Logging standardized
[ ] Consider refactoring references to calendar items by QSharedPointers vs. itemID, standardize all methods according.

### Stage 2: Calendar and Todo
[ ] Begin creating proper UI views for everything
[ ] Create list of program options, implement a proper config dialog. 



## What will satisfy me?
I want a better todo list application and calendar. So what do I really want? A program that I take public and then become maintainer for? I feel like I'd have a lot of caveats. 

It seems that the all-day events I added, with category "income", to signify tax rebate days have been automatically set to "blocks incoming events for me". Now, I don't actually know at present what flag it was that set this. But I need to research this because that shit is retarded. We need profiles of "types" of things scrawled on the calendar. 

It's Saturday, and I'm looking at "Week View" in KOrganizer, and why the hell would I want to see six past days in that? 


#### Immediate work:

March 22nd,
[ ] Finish getting SessionManager implemented.
[ ] Get file-writing working in LocalBackend
[ ] Refactor logic out of over-bloated classes: MainWindow editing stuff into dedicated "EditContext" 


* We need to move a lot of logic out of `CollectionController` into... where?
* We need to move a lot of logic out of `MainWindow` into `CollectionController`.
* We need to improve our editor pane—in fact, we need some sort of unified view interface, such that our `CalendarView`, our editor pane, and any other possible view and editor have a shared underlying representation of the collection and calendar data, all reacting and updating simultaneously to the same signals.
* We need to think about multi-select and multi-edit for batch operations.
* We need to think about clipboard operations (Copy, Cut, Paste). 
* We need a stage pane for uncommited changes which are currently stored as delta data.
* We need to introduce the ability to add and delete `CalendarItems`, and editing, adding, and deleting `Cal`s too.
* We need to think about editing times and dates and item progress.
* We need to start thinking about Todos, including tracking parentage of nested/tree todo items.
* We need to begin *thinking* about sanity checks for data.
* We need to figure out a workflow for synchronization between two backends, exploiting as much as possible what we've already just accomplished for loading and committing collections and deltas to and from memory.
* Once we've fleshed out sync between two `LocalBackends`, we need to update `CalDAVSync`. 

What's immediately obvious is the creation of a new abstract class, `ViewInterface`, which will initially be used by `CalendarView` and a new edit Qwidget for our edit side-pane.

Here's the natural use-case of our MDI interface, as implemented by `MainWindow`:
* when the program opens, the GUI is most limited, allowing only the opening or creation of new collections.
* If a single-backend collection is loaded, a few new items appear to populate the menus/toolbars/statusbar, and the collection information pane, the editor pane, and staged changes pane appear/become active. Everything for editing and commiting changes to and from memory to the backend become possible.
* So long as *any* *one* MDI sub-window which provides a `CalendarView`-based view is open, the collection is open. As soon as the last view onto the collection is closed, the collection itself is "closed"—there can be, of course, warning about unsaved changes, option to discard delta information, etc. to make it clear to the user that they are closing what they've opened. Once closed, the program returns to its empty state, just like it was upon fresh execution.
* If a double-backend collection is loaded, or if a second-backend is attached to a single-backend collection, then *more* options appear. These would be the sync options. 

It seems to me, by making the presence of one `CalendarView` based mdi sub-window essential for a calendar to be "open" means that we can redistribute a lot of signal work from `MainWindow` and `CollectionController` into `CalendarView` instead. `MainWindow` would then become responsble for "hosting" the collections, and displaying or enabling the windows, various panes, and menu/toolbar actions which are appropriate to each context.


## march 23rd update
I told the following to Grok. Now that we actually have rudimentary deltas working, it is a fantastic concretization of the editing workflow. He gave me his grandplan later:

This is a very logical progression for the app. Good. I'm very glad that you thought of Undo and Redo—the delta information we are keeping is absolutely invaluable for that!

Here's my proposed criterion for differentiating between two different kinds of view which are derived from `CalendarView`, which we might call "proactive" and "passive". The proactive view is always an MDI sub-window. At least one of these need to be open in order for a collection to be said to be open. What discriminates them from a passive `CalendarView` is that they can set the active focus or target(s) for an edit. The passive `CalendarView` windows, meanwhile, merely only change upon the updates of focus from proactive views. They facilitate different views and editing capabilities for whatever `CalendarItem` or `Cal` (or, one day, `Collection`) which is in focus.

When multiple proactive `CalendarView` windows are open, the "global" focus for all passive `CalendarView` views is the active selection within the focused (or most recently focused) proactive sub-window. Passive windows change their state upon the focusing, then, of different proactive windows, each of which may have different `CalendarItem`s or `Cal`s selected.

The `EditPane` will be a passive `ViewInterface`, then, for instance. If the user selects an item in a proactive sub- window, begins an edit of that selected item, and then moves to select a different item in the same proactive `ViewInterface`, then the `EditPane` should warn them about unapplied changes. And if they change focus to a *different* proactive sub-window, then what happens?

Oh! I know! In that case, we provide an option to spawn a new *proactive* editor sub-window, carrying over all the changes from the passive pane. So they can apply changes, discard changes, or continue editing in new window. Once the new active edit window is created, the passive edit window now shows a view onto the old data being modified, since that's its job: to show the content of whatever item is in focus for the in-focus window.

And, I suppose, the items which are now the subject of the proactive edit sub-window become locked to editing by other active windows. That's probably a rule which should apply to edits made directly in all proactive windows—they block edits by all over `ViewInterface`s for those items. 

Good god am I jumping through a lot of hoops to avoid modal dialogs! Jeeze!

Okay, so multiselect: when a proactive `ViewInterface` begins a multiselection, the editor pane must become merely a viewer. It can just have a big button saying "edit multiple items", and then the edit of multiple items can be done in a dedicated proactive multi-edit subwindow, locking them for other `ViewInterface` instances just like before.

Okay! How is my reasoning here? Am I missing anything?

Now, it may be possible to have passive "ViewInterface" MDI subwindows in some hypothetical future user workflow. They would just be closed automatically should the last active subwindow be closed.
