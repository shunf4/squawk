# Changelog

## Squawk 0.1.4 (UNRELEASED)
### New features
- several ways to manage your account password:
  - store it in plain text with the config (like it always was)
  - store it in config jammed (local hashing with the constant seed, not secure at all but might look like it is)
  - ask the account password on each program launch
  - store it in KWallet which is dynamically loaded
  
### Bug fixes
- never updating MUC avatars now update
- going offline related segfault fix


## Squawk 0.1.3 (Mar 31, 2020)
### New features
- delivery states for the messages
- delivery receipts now work for real
- avatars in conferences
- edited messages now display correctly
- restyling to get better look with different desktop themes

### Bug fixes
- clickable links now detects better
- fixed lost messages that came with no ID
- avatar related fixes
- message carbons now get turned on only if the server supports them
- progress spinner fix
- files in dialog now have better comment


## Squawk 0.1.2 (Dec 25, 2019)
### New features
- icons in roster for conferences
- pal avatar in dialog window
- avatars next to every message in dialog windows (not in conferences yet)
- roster window position and size now are stored in config
- expanded accounts and groups are stored in config
- availability (from top combobox) now is stored in config

### Bug fixes
- segfault when sending more then one attached file
- wrong path and name of saving file
- wrong message syntax when attaching file and writing text in the save message
- problem with links highlighting in dialog
- project building fixes


## Squawk 0.1.1 (Nov 16, 2019)
A lot of bug fixes, memory leaks fixes
### New features
- exchange files via HTTP File Upload
- download VCards of your contacts
- upload your VCard with information about your contact phones email addresses, names, career information and avatar
- avatars of your contacts in roster and in notifications


## Squawk 0.0.5 (Oct 10, 2019)
### Features
- chat directly
- have multiple accounts
- add contacts
- remove contacts
- assign contact to different groups
- chat in MUCs
- join MUCs, leave them, keep them subscribed or unsubscribed
- download attachmets
- local history
- desktop notifications of new messages
