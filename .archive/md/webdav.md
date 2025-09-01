## 🧩 Combine with `filebrowser`

You can mount the same host directory into both containers (filebrowser and WebDAV), e.g.:

```
yamlCopyEdit# docker-compose.yml
version: '3'

services:
  filebrowser:
    image: filebrowser/filebrowser:simple
    volumes:
      - /data/shared:/srv
    ports:
      - "8080:80"

  webdav:
    image: bytemark/webdav
    environment:
      - USERNAME=myuser
      - PASSWORD=mypass
    volumes:
      - /data/shared:/var/webdav
    ports:
      - "8081:80"
```

------

Would you like a complete `docker-compose.yml` you can copy and run?