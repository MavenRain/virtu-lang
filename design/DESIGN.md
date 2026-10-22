# virtu-lang domain design

> Paste the domain design below the line, verbatim. Do not edit it.

This file is the only source of the core types and the core operations of
virtu-lang. `SPEC.md` sections 5 and 6 cite its sections. Do not add a core
type or a core operation that this file does not give.

When the design changes, paste the new version. Then record the change as an
open item in `SPEC.md` section 9.

---

## The request (verbatim, 2026-10-22)

I want to build a programming language with the following features:

As far as type constructors and higher-kinded types, the language shall admit only cell monads and their duals in a locally presentable locally cartesian closed $(\infty,1)$-category as realized computationally in Cubical Type Theory.

As far as base types and operations, admit only what is provided in the following images:

[Image #29] [Image #30]

The host language shall be TinyCC, and the compilation output shall be the JSON serialization of the final states of each instance of types created in the program.

## The images (transcribed)

The request gives two images. The tables below transcribe them in order.
Each cell keeps the image text, with one change: the image writes the
`create_virtucard` row as "Factory", an em-dash, then "create a card".
This file writes a colon in place of the em-dash (house rule: no
em-dashes).

### Image #29

#### `Avatar`

| Method / Field | Description |
|---|---|
| `Avatar(name, role)` | Create a new avatar |
| `avatar.name` | Avatar's display name |
| `avatar.role` | Avatar's role (`explorer`, `merchant`, …) |
| `avatar.virtucard` | Attached VirtuCard (set explicitly) |
| `avatar.set_attribute(key, value)` | Set a custom attribute |
| `avatar.get_attribute(key)` | Get a custom attribute |
| `avatar.get_inventory()` | List of owned items |

#### `VirtuCard`

| Method / Field | Description |
|---|---|
| `create_virtucard(avatar, amount)` | Factory: create a card |
| `card.balance` | Current balance |
| `card.security_level` | Security level string |
| `card.tap_denominator(amount)` | Add bonus credits |
| `card.debit(amount)` | Deduct credits |
| `card.credit(amount)` | Add credits |
| `card.set_security(level)` | `low` / `medium` / `high` / `maximum` |
| `card.get_history()` | Get transaction history list |
| `transfer(from_card, to_card, amount)` | Transfer between cards |

### Image #30

#### `VirtualTerminal`

| Method / Field | Description |
|---|---|
| `VirtualTerminal(name, category)` | Create a marketplace |
| `terminal.list_item(name, price, desc)` | List item by name + price |
| `terminal.list_item(virtual_item)` | List a `VirtualItem` object |
| `terminal.get_inventory()` | Get all listings |
| `terminal.purchase_item(buyer, name, card)` | 3-arg purchase |
| `terminal.purchase(name, card)` | 2-arg shorthand |

#### `VirtualItem`

| Method / Field | Description |
|---|---|
| `VirtualItem(name, type, base_value)` | Create an item |
| `item.set_rarity(level)` | `common` · `uncommon` · `rare` · `epic` · `legendary` |
| `item.calculate_value()` | `base_value × rarity multiplier` |
| `item.set_attribute(key, value)` | Custom attribute |
| `item.get_attribute(key)` | Retrieve attribute |
