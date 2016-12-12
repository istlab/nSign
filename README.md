How to Train your Browser: Preventing XSS Attacks Using Contextual Script Fingerprints
=====

Cross-Site Scripting (XSS) is one of the most common web application vulnerabilities.
It is therefore sometimes referred to as the “buffer overflow of the web”.
Drawing a parallel from the current state of practice in preventing unauthorized native
code execution (the typical goal in a code injection),
we propose a script whitelisting approach to tame JavaScript-driven XSS attacks.
Our scheme involves a transparent script interception layer placed in the browser's JavaScript engine.
This layer is designed to detect every script that reaches the browser,
from every possible route, and compare it to a list of valid scripts
for the site or page being accessed; scripts not on the
list are prevented from executing.
To avoid the false positives caused by minor syntactic changes
(e.g., due to dynamic code generation),
our layer uses the concept of contextual fingerprints when comparing scripts.

Contextual fingerprints are identifiers that represent
specific elements of a script and its execution context.
Fingerprints can be easily enriched with new elements if needed,
to enhance the proposed method's robustness.
The list can be populated by the website’s administrators,
or a trusted third party.
To verify our approach, we have developed a prototype and tested
it successfully against an extensive array of attacks
that were performed on more than 50 real-world vulnerable web applications.
We measured the browsing performance overhead of the proposed
solution on eight websites that make heavy use of JavaScript.
Our mechanism imposed an average overhead of
11.1% on the execution time of the JavaScript engine.
When measured as part of a full browsing session,
and for all tested websites, the overhead introduced by our layer was less than 0.05%.
When script elements are altered or new scripts are added on the server-side,
a new fingerprint generation phase is required.
To examine the temporal aspect of contextual fingerprints,
we performed a short-term and a long-term experiment based on the same websites.
The former, showed that in a short period of time (ten days),
for seven out of eight websites, the majority of valid fingerprints
stay the same (more than 92% on average).
The latter though, indicated that in the long run the number of
fingerprints that do not change is reduced. Both experiments,
can be seen as one of the first attempts to study the feasibility
of a whitelisting approach for the web.
