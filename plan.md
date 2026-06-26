# Multi-Tab Management Implementation Plan

## Overview

Add multi-tab support to FTB, allowing users to browse multiple directories simultaneously with independent state per tab.

---

## 1. New Files to Create

| File | Purpose |
|------|---------|
| `include/FTB/TabManager.hpp` | `Tab` struct + `TabManager` class definition |
| `src/FTB/TabManager.cpp` | TabManager implementation (create/close/switch tabs) |
| `include/UI/NewTabDialog.hpp` | NewTab dialog header |
| `src/UI/NewTabDialog.cpp` | NewTab dialog (path input + confirm) |
| `include/UI/TabBarRenderer.hpp` | Tab bar rendering header |
| `src/UI/TabBarRenderer.cpp` | Tab bar UI at top of screen |

---

## 2. Existing Files to Modify

| File | Changes |
|------|---------|
| `include/FTB/MainUI.hpp` | Add `TabManager` include, add `ActivePanel::NewTab`, refactor `MainState` to hold a `TabManager` + thin wrappers for per-tab state |
| `include/FTB/KeyBindings.hpp` | Add `PanelCommand::NewTab`, `PanelCommand::CloseTab`, `PanelCommand::NextTab`, `PanelCommand::PrevTab` |
| `src/FTB/KeyBindings.cpp` | Add command aliases: `newtab`/`nt`, `closetab`/`ct`, `nexttab`/`nx`, `prevtab`/`pv` |
| `src/FTB/PanelCommands.cpp` | Add `NewTab` case in `BuildPanelModal()`, `HandlePanelEvent()`, `HandlePanelCommand()`, `RegisterPanelCommands()` |
| `src/FTB/main.cpp` | Initialize `TabManager` in `MainState`, add tab bar to renderer, handle tab switching keys (`[`, `]`, `t`) |
| `src/FTB/Navigation.cpp` | All functions already take `MainState&` — no change needed (they read/write per-tab fields via `state.activeTab()`) |
| `src/FTB/FileListRenderer.cpp` | No structural change — reads per-tab state through `MainState&` |
| `src/FTB/HandleManager/EventHandler.cpp` | Use per-tab clipboard via `TabManager` |
| `src/FTB/SortMode.cpp` | No change — `SortEntries()` already takes a `SortMode` parameter; the caller passes the per-tab mode |
| `src/FTB/PanelCommands.cpp` | Sort dialog reads/writes per-tab sort mode |
| `include/FTB/ClipboardManager.hpp` | Add per-tab clipboard method, or add `TabClipboard` struct inside `Tab` |
| `src/FTB/ClipboardManager.cpp` | Implement per-tab clipboard |
| `CMakeLists.txt` | Add new source files to `FTB_CORE_SOURCES` |
| `include/UI/JumpDirectoryDialog.hpp` | Add tab count display to dialog if desired |
| `src/FTB/StatusBarRenderer.cpp` | Show active tab index in status bar |
| `include/FTB/StatusMessage.hpp` | No change |

---

## 3. Data Structures

### 3.1 `Tab` struct (`include/FTB/TabManager.hpp`)

```cpp
#pragma once
#include <string>
#include <vector>
#include <set>
#include <memory>
#include "FTB/DirectoryHistory.hpp"
#include "FTB/SortMode.hpp"
#include "FTB/FileManager.hpp"

namespace FTB {

struct TabClipboard {
    std::vector<std::string> items;
    bool cutMode = false;
    bool modeSelected = false;

    void addItem(const std::string& path);
    void clear();
    bool isCutMode() const { return cutMode; }
    void setCutMode(bool mode);
    const std::vector<std::string>& getItems() const { return items; }
    bool hasModeSelected() const { return modeSelected; }
    bool paste(const std::string& targetPath);
};

struct Tab {
    std::string currentPath;
    int selected = 0;
    int hovered_index = -1;
    int current_page = 0;
    int total_pages = 1;
    std::vector<std::string> allContents;
    std::vector<std::string> filteredContents;
    std::set<int> batch_selected;
    std::string searchQuery;
    bool search_mode = false;

    // Sort mode per-tab (overrides global config)
    SortMode sortMode = SortMode::NameAsc;
    bool useGlobalSortMode = true;  // if true, read from ConfigManager

    // Navigation history per-tab
    DirectoryHistory directoryHistory;

    // Cached entries per-tab
    std::string cached_current_path_for_entries;
    std::string cached_parent_path;
    std::string cached_parent_display;
    std::string cached_canonical_path;
    int cached_parent_selected = -1;
    std::vector<FileManager::DirEntryInfo> cached_parent_entries;
    std::vector<FileManager::DirEntryInfo> cached_current_entries;

    // Per-tab clipboard
    TabClipboard clipboard;

    // Display name (basename of currentPath)
    std::string displayName() const;

    // Initialize this tab to a given path
    void init(const std::string& path);

    // Refresh directory contents
    void refreshContents();
};

} // namespace FTB
```

### 3.2 `TabManager` class (`include/FTB/TabManager.hpp`)

```cpp
class TabManager {
public:
    TabManager();

    // Create a new tab browsing `path`. Returns index of new tab.
    int createTab(const std::string& path);

    // Close tab at `index`. Returns false if it's the last tab.
    bool closeTab(int index);

    // Switch to tab at `index`. Clamps to valid range.
    void switchTo(int index);

    // Next/prev tab (wraps around)
    void nextTab();
    void prevTab();

    // Access the active tab
    Tab& active();
    const Tab& active() const;

    // Access tab by index
    Tab& tabAt(int index);
    const Tab& tabAt(int index) const;

    // Active tab index
    int activeIndex() const { return activeIndex_; }

    // Number of tabs
    int count() const { return static_cast<int>(tabs_.size()); }

    // Prevent closing last tab
    bool canClose() const { return tabs_.size() > 1; }

private:
    std::vector<Tab> tabs_;
    int activeIndex_ = 0;
};

} // namespace FTB
```

### 3.3 Refactored `MainState` (`include/FTB/MainUI.hpp`)

The key design: `MainState` retains all its existing fields for backwards compatibility during the transition. The `TabManager` is added, and a helper `activeTab()` returns a reference to the current tab. **The per-tab fields on `MainState` become thin aliases that read/write through the active tab.**

To minimize the refactoring blast radius, we use a **convenience access pattern**: rather than changing every call site immediately, we add `TabManager tabManager` to `MainState` and provide `MainState::activeTab()` which returns `Tab&`. The per-tab fields (`currentPath`, `selected`, `filteredContents`, etc.) are kept on `MainState` as **references** that point to the active tab's fields. When the tab switches, these references are re-bound.

However, since C++ doesn't have reference members that can be rebound easily, a better approach is:

**Strategy: Snapshot-on-switch + writeback**

When switching tabs:
1. Save current MainState per-tab fields into the old Tab struct
2. Load the new Tab struct's fields into MainState

This means all existing code continues to work unchanged. The only new code is in `TabManager::switchTo()` which does the save/load dance, and in the renderer/event loop where we call it.

```cpp
// In MainUI.hpp
struct MainState {
    // ... all existing fields (unchanged) ...
    TabManager tabManager;

    // Called when switching tabs — saves current state, loads new tab
    void saveTabState();   // saves per-tab fields from MainState -> active Tab
    void loadTabState();   // loads active Tab's fields -> MainState fields
};
```

---

## 4. Step-by-Step Implementation Order

### Phase 1: Core Tab Data Structure (no UI changes yet)

**Step 1.1: Create `TabManager.hpp` and `TabManager.cpp`**
- Define `TabClipboard`, `Tab`, and `TabManager` as shown in Section 3
- Implement `TabClipboard` (copy logic from existing `ClipboardManager`)
- Implement `Tab::init()`, `Tab::refreshContents()`, `Tab::displayName()`
- Implement `TabManager::createTab()`, `closeTab()`, `switchTo()`, `nextTab()`, `prevTab()`

**Step 1.2: Add `TabManager` to `MainState`**
- In `include/FTB/MainUI.hpp`: `#include "FTB/TabManager.hpp"`, add `TabManager tabManager;` field
- Add `void saveTabState()` and `void loadTabState()` method declarations to `MainState`

**Step 1.3: Implement `saveTabState()` / `loadTabState()`**
- In `src/FTB/MainUI.cpp`:
  ```cpp
  void MainState::saveTabState() {
      auto& tab = tabManager.active();
      tab.currentPath = currentPath;
      tab.selected = selected;
      tab.hovered_index = hovered_index;
      tab.current_page = current_page;
      tab.total_pages = total_pages;
      tab.allContents = std::move(allContents);
      tab.filteredContents = std::move(filteredContents);
      tab.batch_selected = batch_selected;
      tab.searchQuery = searchQuery;
      tab.search_mode = search_mode;
      tab.cached_current_path_for_entries = cached_current_path_for_entries;
      tab.cached_parent_path = cached_parent_path;
      tab.cached_parent_display = cached_parent_display;
      tab.cached_canonical_path = cached_canonical_path;
      tab.cached_parent_selected = cached_parent_selected;
      tab.cached_parent_entries = std::move(cached_parent_entries);
      tab.cached_current_entries = std::move(cached_current_entries);
  }

  void MainState::loadTabState() {
      auto& tab = tabManager.active();
      currentPath = tab.currentPath;
      selected = tab.selected;
      hovered_index = tab.hovered_index;
      current_page = tab.current_page;
      total_pages = tab.total_pages;
      allContents = std::move(tab.allContents);
      filteredContents = std::move(tab.filteredContents);
      batch_selected = tab.batch_selected;
      searchQuery = tab.searchQuery;
      search_mode = tab.search_mode;
      cached_current_path_for_entries = tab.cached_current_path_for_entries;
      cached_parent_path = tab.cached_parent_path;
      cached_parent_display = tab.cached_parent_display;
      cached_canonical_path = tab.cached_canonical_path;
      cached_parent_selected = tab.cached_parent_selected;
      cached_parent_entries = std::move(tab.cached_parent_entries);
      cached_current_entries = std::move(tab.cached_current_entries);
  }
  ```

**Step 1.4: Initialize first tab in `main.cpp`**
- After creating `MainState state` and setting initial path:
  ```cpp
  state.tabManager.createTab(state.currentPath);
  state.loadTabState();
  ```

### Phase 2: Tab Switching in Event Loop

**Step 2.1: Add `t`, `[`, `]` key handling in `main.cpp` event handler**
- Before the keybindings handler:
  ```cpp
  // Tab switching (direct keys, no prefix mode needed)
  if (event == Event::Character('t') && state.active_panel == ActivePanel::None && !state.search_mode) {
      state.active_panel = ActivePanel::NewTab;
      state.panel_input.clear();
      state.panel_message.clear();
      return true;
  }
  if (event == Event::Character(']') && state.active_panel == ActivePanel::None && !state.search_mode) {
      state.saveTabState();
      state.tabManager.nextTab();
      state.loadTabState();
      return true;
  }
  if (event == Event::Character('[') && state.active_panel == ActivePanel::None && !state.search_mode) {
      state.saveTabState();
      state.tabManager.prevTab();
      state.loadTabState();
      return true;
  }
  ```

**Step 2.2: Add tab commands to `PanelCommand` enum**
- In `include/FTB/KeyBindings.hpp`:
  ```cpp
  enum class PanelCommand {
      // ... existing ...
      NewTab,
      CloseTab,
      NextTab,
      PrevTab,
  };
  ```

**Step 2.3: Register command aliases in `KeyBindings.cpp`**
- In `InitCommandMap()`:
  ```cpp
  m["newtab"]    = PanelCommand::NewTab;
  m["nt"]        = PanelCommand::NewTab;
  m["closetab"]  = PanelCommand::CloseTab;
  m["ct"]        = PanelCommand::CloseTab;
  m["nexttab"]   = PanelCommand::NextTab;
  m["nx"]        = PanelCommand::NextTab;
  m["prevtab"]   = PanelCommand::PrevTab;
  m["pv"]        = PanelCommand::PrevTab;
  ```

**Step 2.4: Add `ActivePanel::NewTab` to `MainUI.hpp`**
- Add to enum:
  ```cpp
  enum class ActivePanel {
      // ... existing ...
      NewTab,
  };
  ```

### Phase 3: New Tab Dialog

**Step 3.1: Create `include/UI/NewTabDialog.hpp` and `src/UI/NewTabDialog.cpp`**
- Follow the pattern of `JumpDirectoryDialog`:
  - `RenderNewTabPanel(state, tw, th)` — path input with autocomplete, shows "New Tab" title
  - `HandleNewTabEvent(state, event)` — on Enter, validates path, creates tab via `TabManager::createTab()`, switches to it, closes dialog
  - On Escape, cancel and close dialog

**Step 3.2: Wire into `PanelCommands.cpp`**
- Add `#include "UI/NewTabDialog.hpp"` at top
- In `BuildPanelModal()`: `case ActivePanel::NewTab: return UI::RenderNewTabPanel(state, tw, th);`
- In `HandlePanelEvent()`: `case ActivePanel::NewTab: return UI::HandleNewTabEvent(state, event);`
- In `HandlePanelCommand()`: Add `NewTab`, `CloseTab`, `NextTab`, `PrevTab` cases
- In `RegisterPanelCommands()`: Register callbacks

**Step 3.3: Handle Escape for NewTab in `HandlePanelEvent()`**
- The existing Escape handler in `HandlePanelEvent()` already sets `active_panel = None` and clears inputs — works automatically.

### Phase 4: Tab Bar UI

**Step 4.1: Create `include/UI/TabBarRenderer.hpp` and `src/UI/TabBarRenderer.cpp`**
- `BuildTabBar(TabManager& tabs, int activeIndex)` — returns an FTXUI `Element`
- Render tabs as powerline segments or simple bordered boxes:
  ```
  [ dir1 ] [ ~/Downloads ] [ /etc ] [ /tmp ]
   active   ^^^^^^^^^^^
  ```
- Each tab shows a truncated directory name (basename + parent if duplicate names)
- Active tab highlighted with `TC("selection_bg")` / `TC("selection_fg")`
- Use `PowerlineSegment` for each tab with proper arrow separators
- Click handling: track tab x-ranges, respond to `Mouse::Left` + `Mouse::Pressed`

**Step 4.2: Integrate into `main.cpp` renderer**
- After creating the renderer lambda, before the main_content vbox:
  ```cpp
  auto tab_bar = UI::BuildTabBar(state.tabManager, state.tabManager.activeIndex());
  auto main_content = vbox({
      tab_bar,
      hbox({ parent_col, BuildColumnSeparator(), current_col, ... }) | flex,
      (KeyBindings::GetInstance().IsPrefixMode() || state.search_mode)
          ? BuildCommandBar(state)
          : BuildNormalStatusBar(state)
  }) | bgcolor(TC("main_bg"));
  ```

**Step 4.3: Tab bar height compensation**
- `ComputeLayout()` currently uses `term_h - 4` for items_per_page. With tab bar (1 row), change to `term_h - 5` when tabs > 1.
- Add a `tabBarHeight` parameter or compute inline.

### Phase 5: Per-Tab Clipboard

**Step 5.1: Replace `ClipboardManager` usage with per-tab clipboard**
- In `EventHandler.cpp`, replace:
  ```cpp
  auto& clipboard = ClipboardManager::getInstance();
  ```
  with:
  ```cpp
  auto& clipboard = state.tabManager.active().clipboard;
  ```

- Do the same in `PanelCommands.cpp` for `ClearClipboard` command.

**Step 5.2: Keep the global `ClipboardManager` for cross-tab operations**
- Don't delete the global `ClipboardManager` yet — it can serve as a "cross-tab clipboard" later.
- For now, all copy/cut/paste within a tab uses the tab's own clipboard.

**Step 5.3: Cross-tab file operations (simple approach)**
- Files copied in Tab A remain in Tab A's clipboard. If the user switches to Tab B and pastes, the files are pasted into Tab B's directory from Tab A's source paths (absolute paths are stored, so this works automatically).
- No special cross-tab logic needed — the absolute paths in the clipboard already enable this.
- The clipboard items store absolute source paths, and `paste()` copies/moves them to the target directory. This naturally works across tabs.

### Phase 6: Per-Tab Sort Mode

**Step 6.1: Add `sortMode` field to `Tab`**
- Already included in the `Tab` struct above.

**Step 6.2: Modify `SortDialog.cpp` to use per-tab sort**
- In `HandleSortEvent()`, when saving:
  ```cpp
  // Instead of: cfg.style.sort_mode = SortModeToString(new_mode);
  auto& tab = state.tabManager.active();
  tab.sortMode = new_mode;
  tab.useGlobalSortMode = false;
  ```
- In `RenderSortPanel()`, read from `state.tabManager.active().sortMode` instead of `cfg.style.sort_mode`.

**Step 6.3: Modify `FileManager::getDirectoryEntries()` to accept optional SortMode**
- Add overload: `getDirectoryEntries(const std::string& path, SortMode mode)`
- Or better: have the caller (in `UpdateCurrentEntryCache` and `UpdatePathCache`) pass the sort mode through.
- Simplest approach: the sort mode is read in `getDirectoryEntries` from ConfigManager. To make it per-tab, we need to pass it explicitly. Add a parameter:
  ```cpp
  std::vector<DirEntryInfo> getDirectoryEntries(const std::string& path, SortMode mode);
  ```
- Keep the existing overload for backward compat (reads from config).

**Step 6.4: Thread sort mode through `UpdateCurrentEntryCache` and `UpdatePathCache`**
- These functions read entries. They need to know the sort mode.
- Option A: Add sort mode parameter to `UpdatePathCache(state)` and `UpdateCurrentEntryCache(state)` — but they take `MainState&`, which has the tabManager.
- Option B (simpler): Read sort mode from `state.tabManager.active().sortMode` directly in these functions, or from `state` if we add a `SortMode` accessor.
- **Chosen approach**: Add a `SortMode currentSortMode() const` helper to `MainState`:
  ```cpp
  SortMode MainState::currentSortMode() const {
      auto& tab = tabManager.active();
      if (tab.useGlobalSortMode)
          return SortModeFromString(ConfigManager::GetInstance()->GetConfig().style.sort_mode);
      return tab.sortMode;
  }
  ```
- Modify `UpdateCurrentEntryCache` to use `state.currentSortMode()` instead of reading from config directly.

### Phase 7: Tab Close

**Step 7.1: Add `CloseTab` command handling**
- In `HandlePanelCommand()`:
  ```cpp
  case PanelCommand::CloseTab:
      if (state.tabManager.canClose()) {
          state.saveTabState();
          state.tabManager.closeTab(state.tabManager.activeIndex());
          state.loadTabState();
          StatusMessage::Show("Tab closed");
      } else {
          StatusMessage::Show("Cannot close the last tab");
      }
      break;
  ```

**Step 7.2: Prevent closing last tab**
- `TabManager::closeTab()` returns `false` if `tabs_.size() == 1`.
- The status message informs the user.

### Phase 8: Edge Cases

**8.1: Closing last tab**
- Prevented by `canClose()` check. Show message.

**8.2: Switching to non-existent tab index**
- `TabManager::switchTo()` clamps index to `[0, tabs_.size()-1]`.

**8.3: Creating tab with invalid path**
- `NewTabDialog` validates path before creating tab. Shows error message.

**8.4: Tab with deleted directory**
- When loading a tab, check if `currentPath` still exists. If not, fall back to parent or home.
- Add validation in `Tab::init()` and in `loadTabState()`.

**8.5: File system changes while in another tab**
- When switching to a tab, call `refreshContents()` to get fresh directory listing.

**8.6: Vim editor open during tab switch**
- If `vim_mode_active` is true, prevent tab switch (or close editor first). Since the editor is per-`MainState` (not per-tab), tabs should only switch when editor is closed.

---

## 5. Tab Bar Rendering Detail

The tab bar sits at the very top of the screen, above the three-column layout.

```
┌─────────────────────────────────────────────────────────────┐
│  ~/Documents  │  /etc  │  /tmp  (active highlighted)       │
├─────────────────────────────────────────────────────────────┤
│  parent  │  current  │  preview                            │
│  ...     │  ...      │  ...                                │
```

**Implementation approach:**

```cpp
Element BuildTabBar(TabManager& tabs, int activeIndex) {
    if (tabs.count() <= 1) return text("");  // hide tab bar for single tab

    Elements segments;
    for (int i = 0; i < tabs.count(); ++i) {
        auto& tab = tabs.tabAt(i);
        std::string label = tab.displayName();
        bool isActive = (i == activeIndex);

        Decorator style = isActive
            ? bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold
            : bgcolor(TC("main_bg")) | color(TC("dim"));

        auto segment = text(" " + label + " ") | style;
        if (i < tabs.count() - 1) {
            segment = segment | borderLeft;
        }
        segments.push_back(segment);
    }

    return hbox(std::move(segments));
}
```

**Mouse click handling for tab bar:**
- In `main.cpp` mouse event handler, check if click y == 0 (tab bar row)
- Determine which tab was clicked based on x position
- Save/load tab state, switch tab

---

## 6. Cross-Tab File Operations

The simplest approach (already working by design):

1. User copies files in Tab A → absolute paths stored in `TabA.clipboard.items`
2. User switches to Tab B
3. User presses Ctrl+V → `TabB.clipboard.paste(TabB.currentPath)` is called
4. Wait — this pastes from TabB's clipboard, not TabA's.

**Fix needed:** For cross-tab paste, we need either:
- **Option A**: A shared/global clipboard that all tabs can paste from. Keep the global `ClipboardManager` as-is and add a "paste from global clipboard" command.
- **Option B**: When switching tabs, copy the source tab's clipboard to the new tab's clipboard. This is confusing because it destroys the new tab's own clipboard.
- **Option C (recommended)**: Use a **hybrid approach**. Each tab has its own clipboard, but there's also a "global clipboard" accessible via a separate key (e.g., `Ctrl+G, Ctrl+V` for global paste). Or simpler: just use the global `ClipboardManager` as the default, and per-tab clipboard is opt-in via config.
- **Option D (simplest, recommended)**: Keep using the global singleton `ClipboardManager` for all copy/cut/paste. This naturally supports cross-tab operations since switching tabs doesn't clear the clipboard. The per-tab clipboard is only relevant if the user explicitly wants isolated clipboards — defer this to a future feature.

**Chosen approach: Option D** — Keep using the global `ClipboardManager`. This means cross-tab copy/paste works out of the box. Per-tab clipboard is a future enhancement.

This eliminates Step 5 entirely. The only clipboard change is: no change. Files copied in one tab can be pasted in another because the global clipboard holds absolute paths.

---

## 7. Detailed File Changes

### `include/FTB/MainUI.hpp`

```cpp
// Add after existing includes
#include "FTB/TabManager.hpp"
#include "FTB/SortMode.hpp"

// Add to ActivePanel enum
NewTab,

// Add to MainState struct
TabManager tabManager;

// Add methods
void saveTabState();
void loadTabState();
SortMode currentSortMode() const;
```

### `src/FTB/main.cpp`

Changes:
1. After `state` initialization and path setup, create first tab:
   ```cpp
   state.tabManager.createTab(state.currentPath);
   ```
   Don't call `loadTabState()` here since MainState already has the initial state populated.

2. In renderer lambda, add tab bar above main content.

3. In event handler, add `[`, `]`, `t` handling before keybindings.

4. In mouse handler, add tab bar click detection.

5. In `ComputeLayout()` call, account for tab bar height when tabs > 1.

### `src/FTB/PanelCommands.cpp`

Changes:
1. Add `#include "UI/NewTabDialog.hpp"`
2. Add `NewTab` case in all three switch statements
3. Add `CloseTab`, `NextTab`, `PrevTab` cases in `HandlePanelCommand()`

### `src/FTB/Navigation.cpp`

No changes needed. Functions take `MainState&` and operate on fields that are synced via `saveTabState()`/`loadTabState()`.

### `src/FTB/HandleManager/EventHandler.cpp`

No changes needed (keep global clipboard).

### `CMakeLists.txt`

Add to `FTB_CORE_SOURCES`:
```cmake
src/FTB/TabManager.cpp
src/UI/NewTabDialog.cpp
src/UI/TabBarRenderer.cpp
```

---

## 8. Sort Mode Per-Tab: Detailed Flow

1. User opens Sort dialog (Ctrl+B → sort) in Tab A
2. `HandleSortEvent()` writes to `state.tabManager.active().sortMode`
3. `state.tabManager.active().useGlobalSortMode = false`
4. `UpdateCurrentEntryCache(state)` calls `FileManager::getDirectoryEntries(path, state.currentSortMode())`
5. Entries are sorted with Tab A's sort mode

When user switches to Tab B:
1. `saveTabState()` saves Tab A's state including sortMode
2. `loadTabState()` loads Tab B's state (which may use global sort)
3. Tab B's sort mode is respected

---

## 9. Implementation Priority

| Priority | Phase | Effort | Impact |
|----------|-------|--------|--------|
| P0 | Phase 1: Core TabManager | Medium | Foundation |
| P0 | Phase 2: Tab switching | Low | Core feature |
| P0 | Phase 3: New Tab dialog | Low | Core feature |
| P1 | Phase 4: Tab bar UI | Medium | UX essential |
| P1 | Phase 7: Tab close | Low | Edge case |
| P2 | Phase 6: Per-tab sort | Medium | Nice-to-have |
| P2 | Phase 8: Edge cases | Low | Robustness |
| P3 | Phase 5: Per-tab clipboard | Deferred | Future enhancement |

---

## 10. Testing Strategy

1. **Manual test**: Launch FTB, press `t`, type a path, confirm → new tab appears
2. Press `]` / `[` → tabs switch, file list updates
3. Navigate in one tab, switch to other → independent state
4. Close tab with `Ctrl+B → ct` → tab closes, other tab shown
5. Try closing last tab → message shown, no action
6. Change sort mode in one tab → other tab unaffected
7. Tab bar shows correct active tab
8. Click on tab in tab bar → switch to that tab
9. Vim editor open → tab switch blocked
10. Directory deleted externally → tab handles gracefully
