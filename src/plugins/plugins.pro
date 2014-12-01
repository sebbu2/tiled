TEMPLATE = subdirs
SUBDIRS = csv \
          droidcraft \
          ini \
          flare \
          json \
          lua \
          replicaisland \
          tengine \
          tmw

include(python/find_python.pri)

contains(HAVE_PYTHON, yes) {
    message("Have Python, will slither")
    SUBDIRS += python
} else {
    !build_pass:message("No Python support")
}

