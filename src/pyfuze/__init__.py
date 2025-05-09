from importlib.metadata import PackageNotFoundError, version as _v

try:
    __version__ = _v(__name__)
except PackageNotFoundError:
    __version__ = "0.0.0+dev"

__all__ = ["__version__"]
