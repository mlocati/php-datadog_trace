--TEST--
DDTrace\hook_method returns false with diagnostic when no hook is passed
--ENV--
DD_TRACE_DEBUG=1
--FILE--
<?php

var_dump(DDTrace\hook_method('Greeter', 'greet'));

final class Greeter
{
    public static function greet($name)
    {
        echo "Hello, {$name}.\n";
    }
}

Greeter::greet('Datadog');

?>
--EXPECTF--
DDTrace\hook_method was given neither prehook nor posthook in %s on line %d
bool(false)
Hello, Datadog.
Flushing trace of size 1 to send-queue for %s
