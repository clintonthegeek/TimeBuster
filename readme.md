# TimeBuster

I love KDE, but I have been burned by [Akonadi](https://community.kde.org/KDE_PIM/Akonadi)—the PIM (Personal Information Management) abstraction layer which handles all its calendar functions—so many times that I have become obsessed with the idea of just carving it right out of the stack.

![A Screenshot of TimeBuster](docs/Screenshot_20250422_161218.png?raw=true "TimeBuster")

This is the second attempt at "vibe coding" a calendar program, mostly ping-ponging between Grok and ChatGPT. During the first attempt I developed a very rigid workflow for keeping this structured, and was able to get this thing very, very complex before it all became too complicated to manage and develop further.

I found the work of engineering the software cognitively demanding and getting into a flowstate was extremely rewarding. It was a lot like playing a video game.

Maybe one-day, once I get better and learn more (or when AI gets better), I can pick up and continue hacking on this beast; it has features I haven't seen anywhere else, like granular change tracking saved to the hard drive for every action, etc. It uses KDE libraries like `KCalendarCore` and `KDAV`—unlike my first program, which included its own raw `.ics` parser and `CalDAV` sync implementation, created by feeding entire RFC documents into the LLM!

# License

The use of LLMs to generate code is legally contentious and ethically dubious in the Free Software world. I figure that, if I'm going to be using LLMs this way, releasing all the code I produce this way as GPL3 is the least-bad strategy for doing so.
