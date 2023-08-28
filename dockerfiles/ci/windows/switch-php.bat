IF "%1"=="nts" (
    rd "/php"
    mklink /J "/php" "/php-nts"
) ELSE "%1"=="zts" (
    rd "/php"
    mklink /J "/php" "/php-zts"
) ELSE (
    ECHO "%1 must be nts or zts"
)