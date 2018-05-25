# stooge

Stooge is a small utility designed for easy automated deployment.  It
spins up a very small webserver that listens for GitHub webhooks, and
responds by running shell scripts.

Here's an example of a command to deploy a Hugo blog:

```bash
$ stooge -C /path/to/source -e 'hugo -d /var/www/html'
```
