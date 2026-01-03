# THIS NO LONGER WORKS I MAY TRY TO FIND A NEW METHOD



# 1) Automatically set Gorilla Tag brightness values to MAX which will give you "impossible colors"
# 2) change the Gorilla Tag player name to anything you set just dont put anything bad like the hard r or you will get banned

# REGISTRY TARGET:
HKEY_CURRENT_USER\SOFTWARE\Another Axiom\Gorilla Tag

# VALUES:
- redValue_h2868626173   (REG_QWORD)
- greenValue_h2874538165 (REG_QWORD)
- blueValue_h3443272976  (REG_QWORD)
→ Always set ALL THREE to:
0xFFFFFFFFFFFFFFFF

# PLAYER NAME:
- playerName_h3979151953 (REG_SZ)
- User is prompted once to enter a name
- Trim whitespace
- Remove newlines/tabs
- Max length: 32 characters

