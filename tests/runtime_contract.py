"""Regression cases for the manual-page runtime contract."""
from check_runtime import validate_manual_pages


def page(name, switched, ms):
    return f'PAGE name={name} switched={switched} monotonic_ms={ms} time=22:40 weatherValid=1 stale=0 network=0'


valid = '\n'.join([
    page('clock', 0, 0), page('clock', 0, 3000),
    'BUTTON_PAGE name=weather', page('weather', 1, 5000),
    page('weather', 0, 60000), page('weather', 0, 900000),
    'BUTTON_PAGE name=disaster', page('disaster', 1, 920000),
    page('disaster', 0, 940000),
    'BUTTON_PAGE name=clock', page('clock', 1, 960000),
    page('clock', 0, 980000),
])
assert validate_manual_pages(valid)[1] == 3
invalid = [
    '\n'.join([page('clock', 0, 0), page('weather', 1, 3000)]),
    '\n'.join([page('clock', 0, 0), page('weather', 0, 60000)]),
    '\n'.join([page('clock', 0, 0), 'BUTTON_PAGE name=weather']),
    '\n'.join([page('clock', 0, 0), 'BUTTON_PAGE name=weather', page('clock', 1, 5000)]),
    '\n'.join([page('clock', 0, 0), 'BUTTON_PAGE name=clock', page('clock', 1, 5000)]),
]
for trace in invalid:
    try:
        validate_manual_pages(trace, min_changes=0)
    except AssertionError:
        continue
    raise AssertionError('Invalid manual-page trace was accepted')
print('manual page contract: background refresh and invalid transitions passed')

notification = '\n'.join([page('clock',0,0),
    'DISASTER_ALERT_START count=1 index=0 monotonic_us=1000000 duration_us=3000000',
    page('disaster',0,1000), page('disaster',0,4000), page('disaster',0,60000),
    'DISASTER_ALERT_ACK return_page=clock',
    'BUTTON_PAGE name=clock index=0 count=2 steps=1',page('clock',1,61000)])
assert validate_manual_pages(notification,min_changes=1)[1]==1
print('explicit new-case notification is allowed; unannounced automatic changes remain rejected')
