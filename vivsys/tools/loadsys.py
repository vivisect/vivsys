import sys
import argparse

import vivsys.driver as v_sys_driver

descr = 'Load/Unload windows drivers'
epilog = 'NOTE: subsequent load calls will reload the same driver'

def main():
    parser = argparse.ArgumentParser(description=descr,epilog=epilog)

    parser.add_argument('--unload',dest='unload',default=False,
                        action='store_true',help='*un*load the driver')

    parser.add_argument('--svcname',help='Specify a service name ( default: md5 of driver )')
    parser.add_argument('driver')

    args = parser.parse_args()

    driver = v_sys_driver.Driver(args.driver, name=args.svcname)

    if args.unload:
        print('unloading: %s (service: %s)' % (args.driver,driver.name))
        driver.unload()
    else:
        print('loading: %s (service: %s)' % (args.driver,driver.name))
        driver.load()

if __name__ == '__main__':
    sys.exit(main())
